/** burglerAlarm.cpp
 * Requires an accelerometer by installed.
 * When activated, monitors for movement and sets off the siren if too much
 * movement is detected.
 * 
 * Written by Jotham Gates
 * Last modified 07/11/2022
 */

#include "burglerAlarm.h"

// Global variables that are accessed.
extern TunePlayer tune;
extern BikeHornSound piezo;
extern void startBoost();
extern void stopBoost();

StatesList State::states;
CodeEntry* State::codeEntry;

BurglerAlarmExtension::BurglerAlarmExtension() {
    // Add the burgler alarm function pointer to the menu
    menuActions.length = 1;
    menuActions.array = (MenuItem*)malloc(sizeof(MenuItem));
    menuActions.array[0] = (MenuItem)&BurglerAlarmExtension::stateMachine;
}

void BurglerAlarmExtension::stateMachine() {
    Serial.println(F("Starting state machine"));

    // Create and add the states
    StateInit init;
    State::states.init = &init;
    StateSleep sleep;
    State::states.sleep = &sleep;
    StateAwake awake;
    State::states.awake = &awake;
    StateAlert alert;
    State::states.alert = &alert;
    StateCountdown countdown;
    State::states.countdown = &countdown;
    StateSiren siren;
    State::states.siren = &siren;

    // Add the unlock code
    CodeEntry codeEntry(MY_CODE);
    State::codeEntry = &codeEntry;

    // Run the state machine
    State* current = &init;
    while (current) {
        Serial.println(current->name); // NOTE: For debugging
        current = current->enter();
    }

    // Tidy up
    Serial.println(F("Exiting burgler alarm"));
    // TODO: Beep?

}

// void BurglerAlarmExtension::monitor() {
//     // LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
//     AlarmState state = INIT;
//     uint16_t iterationsRemaining = 40;
//     uint32_t startTime;
//     CodeEntry codeEntry(MY_CODE);
//     while (true) {
//         switch(state) {
//             case INIT:
//                 // Fill up the buffers and go to Low Power
//                 takeReading();
//                 sleepTime = SLEEP_60MS;
//                 iterationsRemaining--;
//                 if(!iterationsRemaining) {
//                     state = SLEEP;
//                 }

//                 break;
            
//             case SLEEP:
//                 // Long sleeps between checks, go to AWAKE when detected.
//                 // TODO: GPIO
//                 if(takeReading()) {
//                     state = AWAKE;
//                     iterationsRemaining = 40;
//                 }
//                 sleepTime = SLEEP_4S;
//                 break;

//             case AWAKE:
//                 // Shorter sleeps between checks, keep accelerometer on. Beep and go to ALERT when detected.
//                 sleepTime = SLEEP_120MS;

//                 iterationsRemaining--; // Nothing happened for a while
//                 if(!iterationsRemaining) {
//                     state = SLEEP;
//                 }
//                 if(takeReading()) {
//                     state = ALERT;
//                     iterationsRemaining = 40;
//                 }
//                 break;
            
//             case ALERT:
//                 // Same as awake, go to COUNTDOWN when detected.
//                 sleepTime = SLEEP_120MS;
//                 iterationsRemaining--; // Nothing happened for a while
//                 if(!iterationsRemaining) {
//                     state = SLEEP;
//                 }
//                 if(takeReading()) {
//                     state = COUNTDOWN_ENTRY;
//                 }
//                 break;

//             case COUNTDOWN_ENTRY:
//                 // Code that runs at the start of countdown
//                 state = COUNTDOWN;
//                 startTime = millis();
//                 iterationsRemaining = 10;
//                 codeEntry.start();
//                 break;

//             case COUNTDOWN:
//                 // Give 10s to enter code. If successful, return. Else go to siren.
//                 if(curTune - startTime >= 1000) {
//                     // Once per second
//                     iterationsRemaining--;
//                     startTime = curTune;
//                 }
//                 if(!iterationsRemaining) {
//                     // Run out of time with no success.
//                     state = SIREN;
//                 }
//                 break;

//             default: // SIREN
//                 // Noise. If button is pressed, go to countdown while still making noise.
//                 // TODO
//                 Serial.println("Siren");
//         }
//         if (sleepTime != SLEEP_SKIP) {
//             LowPower.powerDown(sleepTime, ADC_OFF, BOD_OFF); // The horn will spend most of its life here
//         }
//     }
// }

// bool BurglerAlarmExtension::takeReading() {
//     return false; // TODO
// }

State* StateInit::enter() {
    // TODO: Fill up buffer
    return states.sleep;
}

State* StateSleep::enter() {
    // TODO: Sleep
    stopBoost();
    return states.awake;
}

State* StateAwake::enter() {
    // TODO
    return states.alert;
}

State* StateAlert::enter() {
    // TODO
    return states.countdown;
}

State* StateCountdown::enter() {
    // Starting code entry / countdown
    codeEntry->start();
    if(codeEntry->playWithTune(const_cast<uint16_t*>(alarmCountdownTune))) {
        // Code successful
        Serial.println(F("Success"));
        return nullptr; // To fall through and exit the state machine
    } else {
        // Timed out or fail
        Serial.println(F("Fail"));
        return states.siren;
    }
}

State* StateSiren::enter() {
    Serial.println(F("Siren"));
    // Am not restarting code entry as there are situations where the deadline
    // is just missed and the siren starts, so keep going to finish.
    startBoost();
    if(codeEntry->playWithTune(const_cast<uint16_t*>(BURGLER_ALARM_TUNE))) {
        // Code entered successfully
        return nullptr; // Exit the alarm.
    } else {
        return states.sleep; // Wait for another detection.
    }
}

CodeEntry::CodeEntry(const uint16_t code) : m_code(code) {
    start();
}

CodeEntry::CodeEntry(const uint16_t pin, const uint8_t length) : m_code(ENCODE_CODE(pin, length)) {
    start();
}

void CodeEntry::start() {
    m_charsLeft = m_code & 0xf;
    m_inputted = 0;
    m_lastPressedTime = millis();
}

bool CodeEntry::add(bool character) {
    m_charsLeft--;
    if(m_charsLeft >= 0) {
        m_inputted |= character << m_charsLeft;
        return m_charsLeft == 0;
    }
    return true;
}

bool CodeEntry::update() {
    uint32_t curTime = millis();
    if(curTime - m_lastPressedTime > DEBOUNCE_TIME) {
        // A Button hasn't been pressed for a while.
        if (IS_PRESSED(BUTTON_HORN)) {
            m_lastPressedTime = curTime;
            return add(true);
        } else if(IS_PRESSED(BUTTON_MODE)) {
            m_lastPressedTime = curTime;
            return add(false);
        }

    } else if(IS_PRESSED(BUTTON_MODE) || IS_PRESSED(BUTTON_HORN)) {
        // A button is currently pressed
        m_lastPressedTime = curTime;
    }
    return false;
}

bool CodeEntry::check() {
    return (m_code & ~0xf) == (m_inputted << 4);
}

bool CodeEntry::playWithTune(uint16_t *background_tune) {
    // Starting code entry / countdown
    Serial.println("Waiting for code");
    uiBeep(background_tune);

    // Alternate tune playing
    SoundGenerator mute; // Does nothing
    FlashTuneLoader alternateLoader;
    TunePlayer alternatePlayer;
    // Don't want to call begin to reinitialise the sound generator
    alternateLoader.begin();
    alternateLoader.setTune(const_cast<uint16_t*>(beeps::error));
    alternatePlayer.tuneLoader = &alternateLoader;
    alternatePlayer.soundGenerator = &piezo;

    // Countdown
    while (tune.isPlaying()) {
        WATCHDOG_RESET;
        tune.update();
        alternatePlayer.update();
        // TODO: Flash leds or something

        // Code entry
        if(update()) {
            // Had enough characters entered
            if(check()) {
                // Match
                break;
            } else {
                // Fail.
                Serial.println(F("Failed attempt"));
                // Fail beep
                tune.soundGenerator = &mute; // Ignore the current tune
                alternateLoader.setTune(const_cast<uint16_t*>(beeps::error));
                alternatePlayer.soundGenerator = &piezo; // Connect alternate player
                alternatePlayer.play();

                // Restart code entry
                start();
            }
        }

        // Reconnect countdown tune when alternate beep finished
        if ((!alternatePlayer.isPlaying()) && (tune.soundGenerator != &piezo)) {
        // if (alternatePlayer.isPlaying()) { // TODO: If left in, locks up
            // Finished playing this iteration
            alternatePlayer.stop(); // Resets the tune program counter
            tune.soundGenerator = &piezo;
        }
        
    }
    return check();
}