chrome.runtime.onMessage.addListener((msg) => {
    if (msg.type === "PLAY_SOUND") {
        try {
            const audio = new Audio(chrome.runtime.getURL("alarm_beep.mp3"));
            audio.volume = 0.8;
            audio.play();

            audio.addEventListener("ended", () => {
                window.close();
            });
        } catch (e) {
            console.error("Error playing audio: ", e);
        }
    }
})