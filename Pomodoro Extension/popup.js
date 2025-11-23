/**
 * Overall logic:
 * Start with the "Start" button (or Adjust Focus/Rest Time [optional]), which will start counting down from the focus time
 * When the focus time reaches 0, the program sends a notification, then it will start tracking "overtime"
 * At any time, the user can click "Reset" to reset the timer to the last adjusted focus time (will default to 25 minutes, or their input)
 * The user will at any point click "Swap" to go to the rest timer, which will start a new interval counting down from the rest time plus overtime
 * If the user clicks reset at this time, it will reset restSeconds to lastTrackedRestTime, totalSeconds to lastTrackedFocusTime, then
 * reset the interval and once again set it to totalSeconds with rest time.
 * 
 * Once in Rest mode, the user can at any time click swap, and the next time they enter Rest mode, all time is banked to the second session
 * 
 * NOTES: if timer is running, and "focus" or "rest" button is clicked, it will just add/subtract the respective time from the current timer
 */

/**
 * TODOs for this project
 *  find logic for making 1: settings where we can store more buttons, as a popup, 2: a theme section
 *  make frontend UI look better (combining buttons n such)
 */

/**
 * more ideas:
 *  sliding volume control for alarm sound
 *  different alarm sounds to choose from
 */

let totalSeconds = 1500;
let lastTrackedFocusTime = 1500;
let lastTrackedRestTime = 300;
let startTime = null;
let isFocusMode = true;
let timerInterval = null;
let restSurplus = 0;
let iconPath = "./icons/play.svg";

let timerStarted = false;
document.addEventListener('DOMContentLoaded', async function() {
    console.log("DEBUG: DOM Content Loaded");
    const container = document.getElementById('container');
    container.classList.toggle('hidden');
    const startBtn = document.getElementById('startBtn');
    const resetBtn = document.getElementById('resetBtn');
    const restBtn = document.getElementById('restBtn');
    const adjustBtn = document.getElementById('adjustBtn');
    const swapBtn = document.getElementById('swapBtn');
    const startIcon = document.getElementById('startIcon');
    const settingsBtn = document.getElementById('settingsBtn');
    const backBtn = document.getElementById('backBtn');
    const settings = document.getElementById('settings');

    const signalBox = document.getElementById('signal');
    const display = document.getElementById('display');
    const mode = document.getElementById('mode');
    const overtime = document.getElementById('overtime');

    function updateDisplay() {
        if ((!isFocusMode) == (totalSeconds > 0)) {
            signalBox.style.backgroundColor = "lightgreen";
        } else {
            signalBox.style.backgroundColor = "red";
        }
        if (totalSeconds < 0) {
            overtime.textContent = "OVERTIME";
        } else {
            overtime.textContent = "";
        }
        const absTotalSeconds = Math.abs(totalSeconds);
        const hours = String(Math.floor(absTotalSeconds / 3600)).padStart(2, '0');
        const minutes = String(Math.floor((absTotalSeconds % 3600) / 60)).padStart(2, '0');
        const seconds = String(absTotalSeconds % 60).padStart(2, '0');
        display.textContent = `${hours}:${minutes}:${seconds}`;
    }

    console.log("DEBUG: startup data loaded");
    const startupResults = await chrome.storage.local.get(["totalSeconds", "timerStarted", "isFocusMode", "restSurplus", "startTime", "lastTrackedFocusTime", "lastTrackedRestTime"]);
    lastTrackedFocusTime = startupResults.lastTrackedFocusTime || 1500;
    lastTrackedRestTime = startupResults.lastTrackedRestTime || 300;
    isFocusMode = startupResults.isFocusMode ?? true;
    totalSeconds = startupResults.totalSeconds || ((isFocusMode)?lastTrackedFocusTime:lastTrackedRestTime);
    timerStarted = startupResults.timerStarted ?? false;
    startTime = startupResults.startTime;
    restSurplus = startupResults.restSurplus || 0;
    mode.textContent = (isFocusMode)?"Focus" : "Rest";
    if (!timerStarted) {
        updateDisplay();
    } else {
        const differenceInSeconds = Math.floor((Date.now() - startTime) / 1000);
        totalSeconds = totalSeconds - differenceInSeconds;

        updateDisplay();

        iconPath = "./icons/pause.svg";
        startIcon.src = iconPath;
        
        timerInterval = setInterval(() => {
            totalSeconds--;
            updateDisplay();
        }, 1000);
    }

    settingsBtn.addEventListener('click', function() {
        settings.classList.toggle('hidden');
        container.classList.toggle('hidden');
    });

    backBtn.addEventListener('click', function() {
        settings.classList.toggle('hidden');
        container.classList.toggle('hidden');
    });

    // TODO maybe make timer fire 10 seconds before finishing if we're in rest mode, so overtime doesn't add +45 for being slightly late
    startBtn.addEventListener('click', async function () {
        if (!timerInterval) {
            console.log("DEBUG: start button pressed");

            iconPath = "./icons/pause.svg";
            startIcon.src = iconPath;

            if (timerInterval) return;
            startTime = Date.now();
            timerStarted = true;
            await chrome.storage.local.set({ startTime: startTime, timerStarted: timerStarted });
            if (totalSeconds > 0) {
                chrome.runtime.sendMessage({ type: "SET_TIMER", duration: totalSeconds * 1000 });
            }
            timerInterval = setInterval(() => {
                totalSeconds--;
                updateDisplay();
            }, 1000);
        } else {
            console.log("DEBUG: pause button pressed");

            iconPath = "./icons/play.svg";
            startIcon.src = iconPath;

            timerStarted = false;
            clearInterval(timerInterval);
            timerInterval = null;
            updateDisplay();
            chrome.runtime.sendMessage({ type: "CLEAR_TIMER" });
            await chrome.storage.local.set({ totalSeconds: totalSeconds, timerStarted: timerStarted });
        }
    });

    resetBtn.addEventListener('click', async function() {
        console.log("DEBUG: reset button pressed");
        iconPath = "./icons/play.svg";
        startIcon.src = iconPath;
        clearInterval(timerInterval);
        mode.textContent = "Focus";
        timerInterval = null;
        totalSeconds = lastTrackedFocusTime;
        timerStarted = false;
        isFocusMode = true;
        updateDisplay();
        chrome.runtime.sendMessage({type: "CLEAR_TIMER"});
        await chrome.storage.local.set({ totalSeconds: totalSeconds, timerStarted: timerStarted, restSurplus: 0, isFocusMode: isFocusMode });
    });

    swapBtn.addEventListener('click', async function() {
        console.log("DEBUG: swap button pressed");
        if (isFocusMode) {
            if (totalSeconds > 0) return;
            iconPath = "./icons/pause.svg";
            startIcon.src = iconPath;
            clearInterval(timerInterval);
            timerInterval = null;
            mode.textContent = "Rest";
            isFocusMode = false;
            timerStarted = true;
            let addToRest = -totalSeconds / (lastTrackedFocusTime / lastTrackedRestTime);
            const result  = await chrome.storage.local.get("restSurplus");
            const surplus = result.restSurplus || 0;
            totalSeconds = lastTrackedRestTime + Math.floor(addToRest) + surplus;
            startTime = Date.now()
            await chrome.storage.local.set({ startTime: startTime, isFocusMode: isFocusMode, totalSeconds: totalSeconds, restSurplus: 0, timerStarted: timerStarted });
            chrome.runtime.sendMessage({ type: "SET_TIMER", duration: totalSeconds * 1000 });
            timerInterval = setInterval(() => {
                totalSeconds--;
                updateDisplay();
            }, 1000);
        } else {
            // different logic: we don't care about overtime, and any time remaining time in totalSeconds is "banked" (remove from focus time?)
            iconPath = "./icons/pause.svg";
            startIcon.src = iconPath;
            clearInterval(timerInterval);
            timerInterval = null;
            mode.textContent = "Focus";
            isFocusMode = true;
            timerStarted = true;
            if (totalSeconds < 0) { // overtime during rest period
                restSurplus = 0;
                totalSeconds = lastTrackedFocusTime + Math.floor(-totalSeconds * (lastTrackedFocusTime / lastTrackedRestTime));
            } else {
                restSurplus = totalSeconds;
                totalSeconds = lastTrackedFocusTime;
            }
            startTime = Date.now()
            await chrome.storage.local.set({ timerStarted: timerStarted, startTime: startTime, isFocusMode: isFocusMode, totalSeconds: totalSeconds, restSurplus: restSurplus });
            chrome.runtime.sendMessage({ type: "SET_TIMER", duration: totalSeconds * 1000});
            timerInterval = setInterval(() => {
                totalSeconds--;
                updateDisplay();
            }, 1000);
        }
    });

    adjustBtn.addEventListener('click', async function() {
        console.log("DEBUG: focus button pressed");
        const results = await chrome.storage.local.get("lastTrackedFocusTime");
        const lastTracked = results.lastTrackedFocusTime || 1500;
        const newFocusTime = parseInt(prompt("Enter new focus time in minutes: ", lastTracked/60), 10) * 60;

        if (!isNaN(newFocusTime) && newFocusTime > 0) {
            if (isFocusMode) totalSeconds = totalSeconds + (newFocusTime - lastTrackedFocusTime);
            lastTrackedFocusTime = newFocusTime;
            if (isFocusMode) updateDisplay(totalSeconds);
            if (timerInterval) { // TODO debug
                startTime = Date.now();
                chrome.runtime.sendMessage({ type: "SET_TIMER", duration: totalSeconds * 1000 });
            }
            await chrome.storage.local.set({ startTime: startTime, totalSeconds: totalSeconds, lastTrackedFocusTime: lastTrackedFocusTime });
        }
    });

    restBtn.addEventListener('click', async function() {
        console.log("DEBUG: rest button pressed");
        const results = await chrome.storage.local.get("lastTrackedRestTime");
        const lastTracked = results.lastTrackedRestTime || 300;
        const newRestTime = parseInt(prompt("Enter new rest time in minutes: ", lastTracked/60), 10) * 60;

        if (!isNaN(newRestTime) && newRestTime > 0) {
            if (!isFocusMode) totalSeconds = totalSeconds + (newRestTime - lastTrackedRestTime);
            lastTrackedRestTime = newRestTime;
            if (!isFocusMode) updateDisplay(totalSeconds);
            if (timerInterval) {
                startTime = Date.now();
                chrome.runtime.sendMessage({ type: "SET_TIMER", duration: totalSeconds * 1000 });
            }
            await chrome.storage.local.set({ startTime: startTime, totalSeconds: totalSeconds, lastTrackedRestTime: lastTrackedRestTime });
        }
    });

    updateDisplay();
});