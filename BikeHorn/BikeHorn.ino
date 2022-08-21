/** BikeHorn.ino
 * Source code for the BikeHorn.
 * 
 * For more details, see README.md or go to
 * https://github.com/jgOhYeah/BikeHorn
 * 
 * Written by Jotham Gates
 * Last modified 09/07/2022
 * 
 * Requires these libraries (can be installed through the library manager):
 *   - Low-Power (https://github.com/rocketscream/Low-Power) - Shuts things down to save power.
 *   - TunePlayer (https://github.com/jgOhYeah/TunePlayer) - Does most of the heavy lifting playing the tunes.
 *   - Queue (https://github.com/SMFSW/Queue) - Required by TunePlayer.
 * 
 * Only required if LOG_RUN_TIME is defined:
 *   - EEPROMWearLevel (https://github.com/PRosenb/EEPROMWearLevel) - Logs to EEPROM.
 */

#include "defines.h"

// Libraries to include.
#include <cppQueue.h>
#include <LowPower.h>
#include <TunePlayer.h>
#include <EEPROM.h>

// Only needed for the watchdog timer (DO NOT enable for Arduinos with the old bootloader).
#ifdef ENABLE_WATCHDOG_TIMER
    #include <avr/wdt.h>
    #define WATCHDOG_ENABLE wdt_enable(WDTO_4S)
    #define WATCHDOG_DISABLE wdt_disable()
    #define WATCHDOG_RESET wdt_reset()
#else
    #define WATCHDOG_ENABLE
    #define WATCHDOG_DISABLE
    #define WATCHDOG_RESET
#endif

// Prototypes so that the extension manager is happy
void uiBeep(uint16_t* beep);
void revertToTune();
uint32_t modeButtonPress();
void startBoost();
inline void stopBoost();

#include "tunes.h"
#include "optimisations.h"
#include "soundGeneration.h"

FlashTuneLoader flashLoader;
BikeHornSound piezo;
TunePlayer tune;
uint8_t curTune = 0;
period_t sleepTime = SLEEP_FOREVER;

#ifdef ENABLE_WARBLE
Warble warble;
#endif

// Stores which pin was responsible for waking the system up.
enum Buttons {NONE, HORN, MODE};
volatile Buttons wakePin;

#include "extensions/extensions.h"

void setup() {
    WATCHDOG_ENABLE;
    sleepGPIO(); // Shutdown the timers if the horn crashed previously
    wakeGPIO();
    Serial.println(F(WELCOME_MSG));
    Serial.print(F("There are "));
    Serial.print(tuneCount);
    Serial.println(F(" tunes installed"));

    // Tune Player
    flashLoader.setTune((uint16_t*)pgm_read_word(&(tunes[curTune])));
    tune.begin(&flashLoader, &piezo);
    tune.spool();

    // Extensions
    extensionManager.callOnStart();

#ifdef ENABLE_WARBLE
    /// Warble mode
    warble.begin(&piezo, WARBLE_LOWER, WARBLE_UPPER, WARBLE_RISE, WARBLE_FALL);
#endif
}

void loop() {
    // Go to sleep if not pressed and wake up when a button is pressed
    if(!IS_PRESSED(BUTTON_HORN)) {
        // Wait for tune (beep) to finish before sleeping.
        while(tune.isPlaying()) {
            tune.update();
            WATCHDOG_RESET;
            if (IS_PRESSED(BUTTON_HORN)) {
                // On rare occasions when the button is pressed during a beep, go to sleep and wake up again quickly.
                tune.stop();
                break;
            }
        }
        Serial.println(F("Going to sleep"));
        extensionManager.callOnSleep();
        sleepGPIO();
        wakePin = NONE;
        // Set interrupts to wake the processor up again when required
        attachInterrupt(digitalPinToInterrupt(BUTTON_HORN), wakeUpHornISR, LOW);
        attachInterrupt(digitalPinToInterrupt(BUTTON_MODE), wakeUpModeISR, LOW);
        WATCHDOG_DISABLE;
        LowPower.powerDown(sleepTime, ADC_OFF, BOD_OFF); // The horn will spend most of its life here
        detachInterrupt(digitalPinToInterrupt(BUTTON_HORN));
        detachInterrupt(digitalPinToInterrupt(BUTTON_MODE));
        WATCHDOG_ENABLE;
        wakeGPIO();
        Serial.println(F("Waking up"));
        extensionManager.callOnWake();
    } else {
        wakePin = HORN;
    }

    // If we got here, a button was pressed
    if(wakePin == HORN) {
        // Play a tune
        extensionManager.callOnTuneStart();
        // Start playing
        startBoost();
#ifdef ENABLE_WARBLE
        if(curTune != tuneCount) {
            // Normal tune playing mode
            tune.play();
        } else {
            // Warble mode
            warble.start();
        }
#else
        tune.play();
#endif

        // Play until the button has not been pressed in the past DEBOUNCE_TIME ms.
        digitalWrite(LED_EXTERNAL, HIGH);
        bool ledState = true;

        uint32_t startTime = millis();
        uint32_t curTime;
        uint32_t ledStart = startTime;
        do {
            WATCHDOG_RESET;

#ifdef ENABLE_WARBLE
            // Update warbling or the tune
            if(curTune != tuneCount) {
                tune.update();
            } else {
                warble.update();
            }
#else
            // Update the tune
            tune.update();
#endif

            // Flash the LED every so often
            if(curTime - ledStart > 125) {
                ledStart = curTime;
                digitalWrite(LED_EXTERNAL, !ledState);
                ledState = !ledState;
            }

            // Check if the button is pressed and reset the time if it is.
            if(IS_PRESSED(BUTTON_HORN)) {
                startTime = curTime;
            }
            curTime = millis();
        } while (curTime - startTime < DEBOUNCE_TIME);

        // Stop playing the tune
#ifdef ENABLE_WARBLE
        if(curTune != tuneCount) {
            tune.stop();
            tune.spool();
        } else {
            warble.stop();
        }
#else
        tune.stop();
        tune.spool();
#endif
        extensionManager.callOnTuneStop();

    } else if (wakePin == MODE) {
        // Change tune as the other button is pressed
        digitalWrite(LED_EXTERNAL, HIGH);

        // Only let go once button is not pushed for more than DEBOUNCE_TIME
        uint32_t pressTime = modeButtonPress();

        // Was this a long or short press?
        if (pressTime < LONG_PRESS_TIME) {
            // Short press, change tune
            curTune++;
#ifdef ENABLE_WARBLE
            if(curTune > tuneCount) { // Use == for the warble mode
                curTune = 0;
            }
            if(curTune != tuneCount) {
                // Change tune as normal
#else
            if(curTune == tuneCount) {
                curTune = 0;
            }
#endif
                Serial.print(F("Changing to tune "));
                Serial.println(curTune);
                flashLoader.setTune((uint16_t*)pgm_read_word(&(tunes[curTune])));

                // Stop tune and setup for the next time
                tune.stop();
                tune.spool();
#ifdef ENABLE_WARBLE
            } else {
                // Don't do anything here and select on the fly
                Serial.println(F("Changing to warble mode"));
            }
#endif
        } else {
            // Long press, display menu for extensions
            extensionManager.displayMenu();
        }
    }
}

/**
 * Puts the GPIO into a low power state ready for the microcontroller to sleep.
 */
void sleepGPIO() {
    // Shut down timers to release pins
    TCCR1A = 0;
    TCCR1B = 0;
    stopBoost();

    // Shutdown serial so it won't be affected by playing with the io lines
    Serial.end();

    // Using registers so everything can be done at once easily.
    DDRB = (1 << PB1) | (1 << PB3); // Set everything except pwm to input pullup
#ifdef ACCEL_INSTALLED
    DDRC = ACCEL_PINS;
    PORTC = ~ACCEL_PINS; // Pull up inputs, set outputs as low.
#else
    DDRC = 0;
    PORTC = 0xff;
#endif

    // Pull all unused pins high to avoid floating and reduce power.
    DDRD = 0x03; // Set serial as an output to tie low
    PORTB = ~((1 << PB1) | (1 << PB3));
    PORTD = 0xfc; // Don't pull the serial pins high
    
    // Keep as outputs
    pinMode(LED_EXTERNAL, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_EXTERNAL, LOW);
    digitalWrite(LED_BUILTIN, LOW);
}

/**
 * Wakes and sets up the GPIO and peripheries.
 */
void wakeGPIO() {
    DDRD = 0x02; // Serial TX is the only output
    PORTD = 0x0E; // Idle high serial
    Serial.begin(SERIAL_BAUD);
    DDRB = (1 << PB1) | (1 << PB3);
    PORTB = 0x00; // Make sure everything is off
    pinMode(LED_EXTERNAL, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
}

/**
 * Interrupt Service Routine (ISR) called when the hirn wakes up with the horn button pressed.
 */
void wakeUpHornISR() {
    wakePin = HORN;
}

/**
 * Interrupt Service Routine (ISR) called when the hirn wakes up with the mode button pressed.
 */
void wakeUpModeISR() {
    wakePin = MODE;
}

/**
 * Sets up and starts the intermediate boost stage.
 */
void startBoost() {
    // Set PB3 to be an output (Pin11 Arduino UNO)
    DDRB |= (1 << PB3);
    
    TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20); // Mode 3, fast PWM, reset at 255
    TCCR2B = (1<< CS20); // Prescalar 1
    OCR2A = IDLE_DUTY; // Enough duty cycle to keep the voltage on the second stage at a reasonable level.
}

/**
 * @brief Stops timer 2, which controls the boost stage.
 * 
 */
inline void stopBoost() {
    TCCR2A = 0;
    TCCR2B = 0;
}

/**
 * @brief Waits until the mode button has been released and returns the time
 * it was pressed in ms. If the horn button is pressed at any time, returns 1.
 * 
 * @return uint32_t 
 */
uint32_t modeButtonPress() {
    // Only let go once button is not pushed for more than DEBOUNCE_TIME
    uint32_t pressTime = millis();
    uint32_t debounceTime = pressTime;
    while(millis() - debounceTime < DEBOUNCE_TIME) {
        WATCHDOG_RESET;
        tune.update();
        if(IS_PRESSED(BUTTON_MODE)) {
            debounceTime = millis();
        }

        // If the horn button is pressed, exit immediately to start playing a tune.
        if(IS_PRESSED(BUTTON_HORN)) {
            wakePin = HORN;
            return 1;
        }
    }
    return millis() - pressTime;
}

/**
 * @brief Stores the current tune, then starts playing a beep / other tune.
 * When this is finished, the original tune will be restored.
 * 
 * @param beep the new tune.
 */
void uiBeep(uint16_t* beep) {
    tune.stop();
    tune.setCallOnStop(revertToTune);
    flashLoader.setTune(beep);
    tune.play();
}

/**
 * @brief Reverts the current tune to the selected one.
 * 
 */
void revertToTune() {
    tune.setCallOnStop(NULL);
    tune.stop();
    // If in warble, mode, don't do anything as the flashLoader object isn't used for that and will be set before next use.
#ifdef ENABLE_WARBLE
    if(curTune != tuneCount) {
        // Normal tune playing mode
        flashLoader.setTune((uint16_t*)pgm_read_word(&(tunes[curTune])));
        tune.spool();
    }
#else
    flashLoader.setTune((uint16_t*)pgm_read_word(&(tunes[curTune])));
#endif
}