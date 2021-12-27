/** BikeHorn.ino
 * Source code for the BikeHorn.
 * 
 * For more details, see README.md or go to
 * https://github.com/jgOhYeah/BikeHorn
 * 
 * Written by Jotham Gates
 * Last modified 27/12/2021
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

// EEPROMWearLevel is only required if LOG_RUN_TIME is defined in defines.h
#ifdef LOG_RUN_TIME
    #include <EEPROMWearLevel.h>
    // TODO: Fix wear level to cope with settings also being stored in EEPROM
#endif

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

#include "tunes.h"
#include "soundGeneration.h"

FlashTuneLoader flashLoader;
BikeHornSound piezo;
TunePlayer tune;
uint8_t curTune = 0;

#ifdef ENABLE_WARBLE
Warble warble;
#endif


#ifdef LOG_RUN_TIME
uint32_t startTime;
#endif

// Stores which pin was responsible for waking the system up.
enum WakePin {none, horn, mode};
volatile WakePin wakePin;

void setup() {
    WATCHDOG_ENABLE;
    sleepGPIO(); // Shutdown the timers if the horn crashed previously
    wakeGPIO();
    Serial.println(F(WELCOME_MSG));
    Serial.print(F("There are "));
    Serial.print(tuneCount);
    Serial.println(F(" tunes installed"));

    // Logging (optional)
#ifdef LOG_RUN_TIME
    EEPROMwl.begin(LOG_VERSION, 2);
    Serial.print(F("Run time logging enabled. Horn has been sounding for "));
    Serial.print(getTime() / 1000);
    Serial.println(F(" seconds."));
    Serial.print(F("The horn has been used "));
    Serial.print(getBeeps());
    Serial.println(F(" times."));

#endif

    // Tune Player
    piezo.begin();
    flashLoader.setTune((uint16_t*)pgm_read_word(&(tunes[curTune])));
    tune.begin(&flashLoader, &piezo);
    tune.spool();

#ifdef ENABLE_WARBLE
    /// Warble mode
    warble.begin(&piezo, WARBLE_LOWER, WARBLE_UPPER, WARBLE_RISE, WARBLE_FALL);
#endif

    // Go to midi synth mode if change mode and horn button pressed or reset the eeprom.
    if(!digitalRead(BUTTON_MODE)) {
#ifdef LOG_RUN_TIME
        if(!digitalRead(BUTTON_HORN)) {
            // Both buttons pressed. If both are pressed for 5 seconds, reset the EEPROM.
            uint32_t startTime = millis();
            bool success = true;
            digitalWrite(LED_BUILTIN, HIGH);
            while(millis() - startTime < 5000) {
                WATCHDOG_RESET;
                if(digitalRead(BUTTON_HORN) | digitalRead(BUTTON_MODE)) {
                    success = false;
                    break;
                }
                digitalWrite(LED_EXTERNAL, HIGH);
                delay(100);
                digitalWrite(LED_EXTERNAL, LOW);
                delay(100);
            }
            digitalWrite(LED_BUILTIN, LOW);

            // Check if we left early or at the full time
            if(success) {
                Serial.println(F("Resetting EEPROM"));
                resetEEPROM();
            }
        } else {
            // Only the change tune button pressed. Midi mode.
            midiSynth();
        }
#else
        midiSynth();
#endif
    }
}

void loop() {
    // Go to sleep if not pressed and wake up when a button is pressed
    if(digitalRead(BUTTON_HORN)) {
        Serial.println(F("Going to sleep"));
        sleepGPIO();
        // Set interrupts to wake the processor up again when required
        attachInterrupt(digitalPinToInterrupt(BUTTON_HORN), wakeUpHornISR, LOW);
        attachInterrupt(digitalPinToInterrupt(BUTTON_MODE), wakeUpModeISR, LOW);
        WATCHDOG_DISABLE;
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); // The horn will spend most of its life here
        detachInterrupt(digitalPinToInterrupt(BUTTON_HORN));
        detachInterrupt(digitalPinToInterrupt(BUTTON_MODE));
        WATCHDOG_ENABLE;
        wakeGPIO();
        Serial.println(F("Waking up"));
    } else {
        wakePin = horn;
    }

    // If we got here, a button was pressed
    if(wakePin == horn) {
        // Play a tune

#ifdef LOG_RUN_TIME
        uint32_t wakeTime = millis();
#endif
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
        uint32_t curTime = millis();
        while (curTime - startTime < DEBOUNCE_TIME) {
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
            static uint32_t ledStart = curTime;
            if(curTime - ledStart > 125) {
                ledStart = curTime;
                digitalWrite(LED_EXTERNAL, !ledState);
                ledState = !ledState;
            }

            // Check if the button is pressed and reset the time if it is.
            if(!digitalRead(BUTTON_HORN)) {
                startTime = curTime;
            }
            curTime = millis();
        }
#ifdef LOG_RUN_TIME
        addTime(millis() - wakeTime);
        addBeep();
#endif
    } else {
        // Change tune as the other button is pressed
        digitalWrite(LED_EXTERNAL, HIGH);
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
#ifdef ENABLE_WARBLE
        } else {
            // Don't do anything here and select on the fly
            Serial.println(F("Changing to warble mode"));

        }
#endif

        // Only let go once button is not pushed for more than DEBOUNCE_TIME
        uint32_t startTime = millis();
        while(millis() - startTime < DEBOUNCE_TIME) {
            WATCHDOG_RESET;
            if(!digitalRead(BUTTON_MODE)) {
                startTime = millis();
            }

            // If the horn button is pressed, exit immediately to start playing a tune.
            if(!digitalRead(BUTTON_HORN)) {
                wakePin = horn;
                break;
            }
        }
    }

    // Stop tune and setup for the next time
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
}

/**
 * Puts the GPIO into a low power state ready for the microcontroller to sleep.
 */
void sleepGPIO() {
    // Shut down timers to release pins
    TCCR1A = 0;
    TCCR1B = 0;
    TCCR2A = 0;
    TCCR2B = 0;

    // Shutdown serial so it won't be affected by playing with the io lines
    Serial.end();

    // Using registers so everything can be done at once easily.
    DDRB = (1 << PB1) | (1 << PB3); // Set everything except pwm to input pullup
    DDRC = 0;

    // Pull all unused pins high to avoid floating and reduce power.
    DDRD = 0x03; // Set serial as an output to tie low
    PORTB = ~((1 << PB1) | (1 << PB3));
    PORTC = 0xff;
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
    wakePin = horn;
}

/**
 * Interrupt Service Routine (ISR) called when the hirn wakes up with the mode button pressed.
 */
void wakeUpModeISR() {
    wakePin = mode;
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

/** Mode for a midi synth. This is blocking and will only exit on reset or if the horn button is pressed. */
void midiSynth() {
    // Flashing lights to warn of being in this mode
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(LED_EXTERNAL, HIGH);

    startBoost();
    uint8_t currentNote;

    while(digitalRead(BUTTON_HORN)) {
        WATCHDOG_RESET;
        uint8_t incomingNote; // The next byte to be read off the serial buffer.
        uint8_t midiPitch; // The MIDI note number.
        uint8_t midiVelocity; //The velocity of the note.

        incomingNote = getByte();
        // Analyse the byte. The byte at this stage should be a status byte. If it isn't, the byte is ignored.
        switch (incomingNote) {
            case B10010000 | MIDI_CHANNEL: // Note on
            midiPitch = getByte();
            if(midiPitch != -1) {
                midiVelocity = getByte();
                if(midiVelocity != -1) {
                    piezo.playMidiNote(midiPitch); // This will handle the conversion into note and octave.
                    digitalWrite(LED_BUILTIN, HIGH);
                    digitalWrite(LED_EXTERNAL, LOW);
                    currentNote = midiPitch;
                }
            }
            break;

            case B10000000 | MIDI_CHANNEL: // Note off
            midiPitch = getByte();
            if(midiPitch == currentNote) {
                piezo.stopSound();
                digitalWrite(LED_BUILTIN, LOW);
                digitalWrite(LED_EXTERNAL, HIGH);
            }
            break;
        }
    }
    piezo.stopSound();
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(LED_EXTERNAL, LOW);
}

/** Wait for an incoming note and return the note. */
byte getByte() {
    while (!Serial.available() && digitalRead(BUTTON_HORN)) {
        WATCHDOG_RESET;
    } //Wait while there are no bytes to read in the buffer.
    return Serial.read();
}

// Stuff only needed if recording run time.
#ifdef LOG_RUN_TIME
/** @returns the time the horn has been sounding in ms */
uint32_t getTime() {
    uint32_t time;
    EEPROMwl.get(0, time);
    return time;
}

/** Add @param time in ms to the total time the horn has been sounding */
void addTime(uint32_t time) {
    time += getTime();
    EEPROMwl.put(0, time);
}

/** @returns the number of times the horn has gone off */
uint16_t getBeeps() {
    uint16_t beeps;
    EEPROMwl.get(1, beeps);
    return beeps;
}

/** Adds 1 to the number of times the horn has gone off */
void addBeep() {
    uint16_t beeps = getBeeps() + 1;
    EEPROMwl.put(1, beeps);
}

/** Resets stored data to 0 */
void resetEEPROM() {
    EEPROMwl.put(0, (uint32_t)0);
    EEPROMwl.put(1, (uint16_t)0);
}
#endif
