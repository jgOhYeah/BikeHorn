#include "defines.h"
#include <cppQueue.h>
#include <Arduino.h>
#include <LowPower.h>
#define MANUAL_CUTOFF
#include <TunePlayer.h>
#include "tunes.h"
#include "soundGeneration.h"

FlashTuneLoader flashLoader;
BikeHornSound piezo;
TunePlayer tune;
uint8_t curTune = 0;

enum WakePin {none, horn, mode};
volatile WakePin wakePin;

void setup() {
    wakeGPIO();
    Serial.println(F(WELCOME_MSG));
    Serial.print(F("There are "));
    Serial.print(tuneCount);
    Serial.println(F(" tunes installed"));

    // Tune Player
    flashLoader.setTune(pgm_read_word(&(tunes[curTune])));
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
    } else {
        // Change tune as the other button is pressed
        digitalWrite(LED_EXTERNAL, HIGH);
        curTune++;
        if(curTune == tuneCount) {
            curTune = 0;
        }
        Serial.print(F("Changing to tune "));
        Serial.println(curTune);
        flashLoader.setTune(pgm_read_word(&(tunes[curTune])));

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