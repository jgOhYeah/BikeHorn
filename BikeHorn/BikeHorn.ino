#include "defines.h"
#include <cppQueue.h>
#include <Arduino.h>
#include <LowPower.h>
#include <TunePlayer.h>
#ifdef LOG_RUN_TIME
    #include <EEPROMWearLevel.h>
#endif
#include "tunes.h"
#include "soundGeneration.h"

// TODO: EEPROM Logging (optional) for battery life analysis.

FlashTuneLoader flashLoader;
BikeHornSound piezo;
TunePlayer tune;
uint8_t curTune = 0;

#ifdef LOG_RUN_TIME
uint32_t startTime;
#endif

enum WakePin {none, horn, mode};
volatile WakePin wakePin;

void setup() {
    wakeGPIO();
    Serial.println(F(WELCOME_MSG));
    Serial.print(F("There are "));
    Serial.print(tuneCount);
    Serial.println(F(" tunes installed"));

    // Logging
#ifdef LOG_RUN_TIME
    EEPROMwl.begin(LOG_VERSION, 4);
    Serial.print(F("Run time logging enabled. Horn has been sounding for "));
    Serial.print(getTime() / 1000);
    Serial.println(F(" seconds."));
#endif

    // Tune Player
    flashLoader.setTune((uint16_t*)pgm_read_word(&(tunes[curTune])));
    tune.begin(&flashLoader, &piezo);
    tune.spool();

    // Go to midi synth mode if change mode and horn button pressed
    if(!digitalRead(BUTTON_MODE)) {
        midiSynth();
    }
}

void loop() {
    // Go to sleep if not pressed and wake up when a button is pressed
    if(digitalRead(BUTTON_HORN)) {
        sleepGPIO();
        // Set interrupts to wake the processor up again when required
        attachInterrupt(digitalPinToInterrupt(BUTTON_HORN), wakeUpHornISR, LOW);
        attachInterrupt(digitalPinToInterrupt(BUTTON_MODE), wakeUpModeISR, LOW);
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
        detachInterrupt(digitalPinToInterrupt(BUTTON_HORN));
        detachInterrupt(digitalPinToInterrupt(BUTTON_MODE));
        wakeGPIO();
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
        tune.play();

        // Play until the button has not been pressed in the past DEBOUNCE_TIME ms.
        digitalWrite(LED_EXTERNAL, HIGH);
        bool ledState = true;

        uint32_t startTime = millis();
        uint32_t curTime = millis();
        while (curTime - startTime < DEBOUNCE_TIME) {
            tune.update();
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
#endif
    } else {
        // Change tune as the other button is pressed
        digitalWrite(LED_EXTERNAL, HIGH);
        curTune++;
        if(curTune == tuneCount) {
            curTune = 0;
        }
        Serial.print(F("Changing to tune "));
        Serial.println(curTune);
        flashLoader.setTune((uint16_t*)pgm_read_word(&(tunes[curTune])));

        // Only let go once button is not pushed for more than DEBOUNCE_TIME
        uint32_t startTime = millis();
        while(millis() - startTime < DEBOUNCE_TIME) {
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
    tune.stop();
    tune.spool();
}

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
    DDRC = 0; // TODO: Remove voltage divider as is will be wasting power.

    // Pull all unused pins high to avoid floating and reduce power.
    DDRD = 0x03; // Set serial as an output to tie low
    PORTB = ~((1 << PB1) | (1 << PB3));
    PORTC = 0xff;
    PORTD = 0xf0; // Don't pull the serial pins and buttons high
    
    // Keep as outputs
    pinMode(LED_EXTERNAL, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_EXTERNAL, LOW);
    digitalWrite(LED_BUILTIN, LOW);
}

void wakeGPIO() {
    DDRD = 0x02; // Serial TX is the only output
    PORTD = 0x02; // Idle high serial
    Serial.begin(38400);
    DDRB = (1 << PB1) | (1 << PB3);
    PORTB = 0x00; // Make sure everything is off
    pinMode(LED_EXTERNAL, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
}

void wakeUpHornISR() {
    wakePin = horn;
}

void wakeUpModeISR() {
    wakePin = mode;
}

// Functions for managing the intermediate boost stage
void startBoost() {
    // Set PB3 to be an output (Pin11 Arduino UNO)
    DDRB |= (1 << PB3);
    
    TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20); // Mode 3, fast PWM, reset at 255
    TCCR2B = (1<< CS20); // Prescalar 1
    OCR2A = IDLE_DUTY; // 76.6% duty
}

/** Mode for a midi synth */
void midiSynth() {
    // Flashing lights to warn of being in this mode
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(LED_EXTERNAL, HIGH);

    startBoost();
    uint8_t currentNote;

    while(digitalRead(BUTTON_HORN)) {
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
    while (!Serial.available() && digitalRead(BUTTON_HORN)) {} //Wait while there are no bytes to read in the buffer.
    return Serial.read();
}

// Stuff only needed if recording run time.
#ifdef LOG_RUN_TIME
uint32_t getTime() {
    return (EEPROMwl.read(0) << 24) + (EEPROMwl.read(1) << 16) + (EEPROMwl.read(2) << 8) + EEPROMwl.read(3);
}

void addTime(uint32_t time) {
    time += getTime();
    EEPROMwl.update(0, (time >> 24) & 0xFF);
    EEPROMwl.update(1, (time >> 16) & 0xFF);
    EEPROMwl.update(2, (time >> 8) & 0xFF);
    EEPROMwl.update(3, time & 0xFF);
}
#endif
