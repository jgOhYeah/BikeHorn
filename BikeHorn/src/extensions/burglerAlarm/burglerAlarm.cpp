#include "Arduino.h"
/** burglerAlarm.cpp
 * Requires an accelerometer by installed.
 * When activated, monitors for movement and sets off the siren if too much
 * movement is detected.
 * 
 * Written by Jotham Gates
 * Last modified 27/01/2023
 */

#include "burglerAlarm.h"

// Global variables that are accessed.
extern TunePlayer tune;
extern BikeHornSound piezo;
extern volatile Buttons wakePin;
extern void startBoost();
extern void stopBoost();
extern void uiBeep(uint16_t* beep);
extern void uiBeepBlocking(uint16_t* beep);
extern void wakeGPIO();
extern void sleepGPIO();
extern void wakeUpEnable();
extern void wakeUpDisable();

// Statis state members
StatesList State::states;
CodeEntry* State::codeEntry;
Acceleromenter* State::accelerometer;

BurglerAlarmExtension::BurglerAlarmExtension() {
    // Add the burgler alarm function pointer to the menu
    menuActions.length = 1;
    menuActions.array = (MenuItem*)malloc(sizeof(MenuItem));
    menuActions.array[0] = (MenuItem)&BurglerAlarmExtension::stateMachine;
}

void BurglerAlarmExtension::onStart() {
    stateMachine();
}

void BurglerAlarmExtension::stateMachine() {
    Serial.println(F("Starting state machine"));
    Serial.flush();

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

    // Add the accelerometer control
    Acceleromenter accelerometer;
    State::accelerometer = &accelerometer;

    // Run the state machine
    State* current = &init;
    while (current) {
        wakeGPIO(); // To enable serial
        Serial.println(current->name); // NOTE: For debugging
        current = current->enter();
    }

    // Tidy up
    accelerometer.stop();
    Serial.println(F("Exiting burgler alarm"));
    // TODO: Beep?

}

State* StateInit::enter() {
    // TODO: Button to cancel
    wakeUpEnable(); // Enable waking up and cancelling if pressed.
    accelerometer->start();
    for (uint8_t i = 0; i != PREVIOUS_RECORDS && wakePin == PRESSED_NONE; i++) {
        WATCHDOG_RESET;
        LowPower.powerDown(SLEEP_250MS, ADC_ON, BOD_OFF); // Also time for startup
        accelerometer->calibrate();
    }
    wakeUpDisable();
    wakeGPIO();
    // Check if the alarm entry has been cancelled.
    if(wakePin == PRESSED_NONE) {
        // Not touched, go to alarm
        uiBeepBlocking(const_cast<uint16_t*>(beeps::acknowledge));
        return states.sleep;
    } else {
        // Touched, cancel alarm
        uiBeepBlocking(const_cast<uint16_t*>(beeps::cancel));
        return nullptr;
    }
}

State* StateSleep::enter() {
    // State setup
    sleepGPIO();
    wakeUpEnable();

    // Main loop
    do {
        WATCHDOG_RESET;

        // Shut down and sleep
        accelerometer->stop();
        LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);

        // Check the button wasn't pressed during the shutdown sleep
        if (wakePin != PRESSED_NONE) {
            break;
        }

        // Turn the accelerometer on and wait for it start up
        accelerometer->powerOn();
        LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);

        // Take the reading
        accelerometer->startADC();
    } while (wakePin == PRESSED_NONE && !accelerometer->isMoved());

    // Exiting the state
    wakeUpDisable();
    if (wakePin == PRESSED_NONE) {
        // Button not pressed.
        return states.awake;
    } else {
        // Button pressed.
        return states.countdown;
    }
}

State* StateAwake::enter() {
    // Start up
    wakeUpEnable();
    accelerometer->powerOn();
    accelerometer->startADC();

    // Monitor the accelerometer at a higher rate for a while.
    for (uint8_t i = 0; i != IGNORE_CYCLES && wakePin == PRESSED_NONE; i++) {
        WATCHDOG_RESET;
        digitalWrite(LED_EXTERNAL, LOW);
        LowPower.powerDown(SLEEP_250MS, ADC_ON, BOD_OFF); // Also time for startup
        digitalWrite(LED_EXTERNAL, HIGH);
        accelerometer->isMoved(); // Ignore for a while
    }
    
    // Time spent or button pressed.
    wakeUpDisable();
    digitalWrite(LED_EXTERNAL, LOW);
    if (wakePin == PRESSED_NONE) {
        // Timed out.
        return states.alert;
    } else {
        // Button pressed.
        return states.countdown;
    }
}

State* StateAlert::enter() {
    // Start up
    wakeUpEnable();
    accelerometer->powerOn();
    accelerometer->startADC();

    // Monitor the accelerometer at a higher rate for a while.
    for (uint8_t i = 0; i != ALERT_CYCLES && wakePin == PRESSED_NONE; i++) {
        WATCHDOG_RESET;
        digitalWrite(LED_EXTERNAL, LOW);
        LowPower.powerDown(SLEEP_250MS, ADC_ON, BOD_OFF); // Also time for startup
        digitalWrite(LED_EXTERNAL, HIGH);
        if (accelerometer->isMoved()) {
            wakeUpDisable();
            return states.countdown;
        }
    }

    // Not moved. Timed out or button pressed.
    wakeUpDisable();
    if (wakePin == PRESSED_NONE) {
        // Timed out.
        return states.sleep;
    } else {
        // Button pressed.
        return states.countdown;
    }
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
    m_lastPressedTime = millis() - DEBOUNCE_TIME - 1; // Subtracting debounce time so that the button being pressed at the start is recognised
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