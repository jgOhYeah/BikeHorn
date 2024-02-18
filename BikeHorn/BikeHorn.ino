/** BikeHorn.ino
 * Source code for the BikeHorn.
 * 
 * For more details, see README.md or go to
 * https://github.com/jgOhYeah/BikeHorn
 * 
 * Written by Jotham Gates
 * Last modified 27/01/2023
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

// Prototypes so that the extension manager is happy
void uiBeep(uint16_t* beep);
void uiBeepBlocking(uint16_t* beep);
void revertToTune();
uint32_t modeButtonPress();
void startBoost();
inline void stopBoost();
void sleepGPIO();
void wakeGPIO();

#include "tunes.h"
#include "src/optimisations.h"
#include "src/soundGeneration.h"
#include "src/soundGenerationStatic.h"

FlashTuneLoader flashLoader;
BikeHornSound piezo;
TunePlayer tune;
uint8_t curTune = 0;

#ifdef NO_BUTTON_INTERRUPTS
period_t sleepTime = SLEEP_60MS;
#else
period_t sleepTime = SLEEP_FOREVER;
#endif

#ifdef ENABLE_WARBLE
Warble warble;
extern void uiBeep();
#endif

volatile Buttons wakePin;

#include "src/extensions/extensions.h"

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

        // Go to sleep until a button is pressed.
        Serial.println(F("Going to sleep"));
        extensionManager.callOnSleep();
        sleepGPIO();
#ifdef NO_BUTTON_INTERRUPTS
        waitForButton();
#else
        wakeUpEnable();
        WATCHDOG_DISABLE;
        LowPower.powerDown(sleepTime, ADC_OFF, BOD_OFF); // The horn will spend most of its life here
        wakeUpDisable();
        WATCHDOG_ENABLE;
#endif

        // Button pressed. Start waking up.
        wakeGPIO();
        Serial.println(F("Waking up"));
        extensionManager.callOnWake();
    } else {
        wakePin = PRESSED_HORN;
    }

    // If we got here, a button was pressed
    if(wakePin == PRESSED_HORN) {
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

    } else if (wakePin == PRESSED_MODE) {
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

#ifdef __AVR_ATmega32U4__
/**
 * Without built in serial pins, setting up for sleep and being active are pretty much the same thing for the ATmega32u4.
 * 
 */
void setupPins() {
    // Port B: Set everything except the TX LED, piezo pwm and external LED to input pullup.
    DDRB = bit(PB0) | bit(PB5) | bit(PB6);
    PORTB = ~(bit(PB0) | bit(PB5) | bit(PB6));

    // Port C: Set the buttons to input pullup.
    DDRC = 0x0;
    PORTC = 0xff;

    // Port D: Set all except the RX Led and boost to input pullup.
    DDRD = bit(PD5) | bit(PD7);
    PORTD = ~(bit(PD5) | bit(PD7));

    // Port E: Set all except the boost pin to input pullup. // TODO: Change boost pin.
    DDRE = bit(PE6);
    PORTE = ~bit(PE6);

    // Port F: Set all to input pullup.
    DDRF = 0x0;
    DDRF = 0xff;
}
#endif

/**
 * Puts the GPIO into a low power state ready for the microcontroller to sleep.
 */
void sleepGPIO() {
    // Shut down timers to release pins.
    // Both ATmega32u4 and ATmega328p.
    TCCR1A = 0;
    TCCR1B = 0;
    stopBoost();

    // Using registers so everything can be done at once easily.
#ifdef __AVR_ATmega32U4__
    // TODO: Does the serial port need special treatment?
    setupPins();
#else
    // Shutdown serial so it won't be affected by playing with the io lines
    Serial.end();

    DDRB = bit(PB1) | bit(PB3); // Set everything except pwm to input pullup
    #ifdef ACCEL_INSTALLED
    DDRC = ACCEL_PINS_SLEEP_MODE;
    PORTC = ACCEL_PINS_SLEEP_STATE; // Pull up unusec inputs, set everything else low.
    #else
    DDRC = 0;
    PORTC = 0xff;
    #endif

    // Pull all unused pins high to avoid floating and reduce power.
    DDRD = 0x03; // Set serial as an output to tie low
    PORTB = ~(bit(PB1) | bit(PB3));
    PORTD = 0xfc; // Don't pull the serial pins high

    // Keep as outputs
    pinMode(LED_EXTERNAL, OUTPUT);
    digitalWrite(LED_EXTERNAL, LOW);
#endif
#ifdef USES_LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
#endif
}

/**
 * Wakes and sets up the GPIO and peripheries.
 */
void wakeGPIO() {
#ifdef __AVR_ATmega32U4__
    setupPins();
#else
    DDRD = 0x02; // Serial TX is the only output
    PORTD = 0x0E; // Idle high serial
    Serial.begin(SERIAL_BAUD);
    DDRB = bit(PB1) | bit(PB3);
    PORTB = 0x00; // Make sure everything is off
    pinMode(LED_EXTERNAL, OUTPUT);
#endif
#ifdef USES_LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);
#endif

#ifdef WAIT_FOR_SERIAL
    while (!Serial);
#endif

    // Will leave waking the accelerometer up to its own code as it isn't needed often.
}

#ifndef NO_BUTTON_INTERRUPTS
void wakeUpEnable() {
    wakePin = PRESSED_NONE;
    // Set interrupts to wake the processor up again when required
    attachInterrupt(digitalPinToInterrupt(BUTTON_HORN), wakeUpHornISR, LOW);
    attachInterrupt(digitalPinToInterrupt(BUTTON_MODE), wakeUpModeISR, LOW);
}

void wakeUpDisable() {
    detachInterrupt(digitalPinToInterrupt(BUTTON_HORN));
    detachInterrupt(digitalPinToInterrupt(BUTTON_MODE));
}

/**
 * Interrupt Service Routine (ISR) called when the hirn wakes up with the horn button pressed.
 */
void wakeUpHornISR() {
    wakePin = PRESSED_HORN;
}

/**
 * Interrupt Service Routine (ISR) called when the hirn wakes up with the mode button pressed.
 */
void wakeUpModeISR() {
    wakePin = PRESSED_MODE;
}
#else
/**
 * The buttons aren't currently connected to interrupt capable pins, so use this instead.
 * 
 */
inline void waitForButton() {
    wakePin = PRESSED_NONE;
    while (true) {
        if(IS_PRESSED(BUTTON_HORN)) {
            wakePin = PRESSED_HORN;
            break;
        }
        if(IS_PRESSED(BUTTON_MODE)) {
            wakePin = PRESSED_MODE;
            break;
        }
        WATCHDOG_RESET;
    #if defined(WAIT_FOR_SERIAL) && defined(__AVR_ATmega32U4__)
        // Don't let the serial port go to sleep
        delay(60);
    #else
        LowPower.powerDown(sleepTime, ADC_OFF, BOD_OFF); // TODO: Cope with other people adjusting sleepTime in extensions as this doesn't behave the same as interrupts.
    #endif
    }
}
#endif

/**
 * Sets up and starts the intermediate boost stage.
 */
void startBoost() {
#ifdef __AVR_ATmega32U4__
    // Set OC4D (PD7) as an output and connect it
    DDRD |= bit(PD7);
    TCCR4A = 0x0;
    TCCR4B = bit(CS40); // Prescalar 1
    TCCR4C = bit(COM4D1) | bit(PWM4D); // Connect OC4D and enable the PWM modulator on 0C4D.
    TCCR4D = 0x0; // WGM bits set to 0. TOP=OCR4C.
    TCCR4E = 0x0; // Some read only values here, others only apply to PWM6 mode.

    // Set the top value to 255 as expected of the atmega328p timer 2.
    TC4H = 0x0; // Set the top 2 bits to 0.
    OCR4C = 0xff;
#else
    // Set PB3 to be an output (Pin11 Arduino UNO)
    DDRB |= (1 << PB3);
    
    TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20); // Mode 3, fast PWM, reset at 255
    TCCR2B = (1<< CS20); // Prescalar 1
    OCR2A = IDLE_DUTY; // Enough duty cycle to keep the voltage on the second stage at a reasonable level.
#endif
    SET_IDLE_DUTY();
}

/**
 * @brief Stops timer 2, which controls the boost stage.
 * 
 */
inline void stopBoost() {
#ifdef __AVR_ATmega32U4__
    TCCR4A = 0x0;
    TCCR4B = 0x0;
    TCCR4C = 0x0;
    TCCR4D = 0x0;
    TCCR4E = 0x0;
#else
    TCCR2A = 0;
    TCCR2B = 0;
#endif
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
            wakePin = PRESSED_HORN;
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
 * @brief Calls uiBeep and waits untill the tune it done.
 * 
 * // TODO: This will probably need to be adapted / removed when button input is added.
 * 
 * @param beep 
 */
void uiBeepBlocking(uint16_t* beep) {
    uiBeep(beep);
    while(tune.isPlaying()) {
        tune.update();
        WATCHDOG_RESET;
    }
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