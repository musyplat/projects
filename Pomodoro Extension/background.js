chrome.runtime.onMessage.addListener((msg) => {
    if (msg.type === "SET_TIMER") {
        chrome.alarms.create("timerAlarm", {delayInMinutes: msg.duration / 60000});
    } else if (msg.type === "CLEAR_TIMER") {
        chrome.alarms.clear("timerAlarm");
    }
});

async function ensureOffscreenDocument() {
    const offscreenUrl = chrome.runtime.getURL("sound.html");
    const hasDoc = await chrome.offscreen.hasDocument?.();
    if (!hasDoc) {
        await chrome.offscreen.createDocument({
            url: offscreenUrl,
            reasons: [chrome.offscreen.Reason.AUDIO_PLAYBACK],
            justification: "Play timer sound",
        });
    }
}

chrome.alarms.onAlarm.addListener(async (alarm) => {
    if (alarm.name === "timerAlarm") {
        chrome.notifications.create({
            type: "basic",
            iconUrl: "default_icon.png",
            title: "Pomodoro Timer",
            message: "Overtime has been started! Press Swap Button to change modes.",
            priority: 2
        });

        await ensureOffscreenDocument();
        setTimeout(() => {
            chrome.runtime.sendMessage({ type: "PLAY_SOUND" });
        }, 50)
    }
});