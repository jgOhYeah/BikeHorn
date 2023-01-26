/** defines.h
 * Definitions and settings for the BikeHorn.
 * 
 * For more details, see README.md or go to
 * https://github.com/jgOhYeah/BikeHorn
 * 
 * Written by Jotham Gates
 * Last modified 01/11/2022
 */

#pragma once
#define VERSION "1.4.0a"

/**
 * @brief Pins and system setup
 * 
 */
#define PIEZO_PIN 9 // Fixed as PB1 (Pin 9 on Arduino Nano)
#define BOOST_PIN 11 // Fixed as PB3 (Pin 11 on Arduino Nano)
#define LED_EXTERNAL 7
#define BUTTON_HORN 2 // Needs to be interrupt capable (pin 2 or 3)
#define BUTTON_MODE 3 // Needs to be interrupt capable (pin 2 or 3)

#define SERIAL_BAUD 38400

#define IDLE_DUTY 5 // 9.4% duty cycle, keeps the voltage up when not playing
#define MIDI_CHANNEL 0 // Zero indexed, so many software shows ch. 0 as ch. 1
#define DEBOUNCE_TIME 20

// Watchdog timer to reduce lockups with a flat battery / unstable power supply
#define ENABLE_WATCHDOG_TIMER // DO NOT enable this (uncomment it) for Arduinos running the older (pre optiboot) bootloader as this will cause lockups

/**
 * @brief Warble mode settings
 * 
 */
#define ENABLE_WARBLE // Define this to add a warble sound as a final tune (like various burgler alarms, sirens and
                      // existing HPV horns)
#define WARBLE_LOWER 3000 // Lower frequency (Hz)
#define WARBLE_UPPER 3800 // Upper frequency (Hz)
#define WARBLE_RISE 15000 // Up chirp time (ms)
#define WARBLE_FALL 50000 // Down chirp time (ms)
#define WARBLE_STEP 10 // Number of timer 1 counts to step at a time (need to keep time between steps high enough to not
                       // get bogged down as each update now takes ~100us with eeprom piecewise)

/**
 * @brief Logging and EEPROM settings
 * 
 */
#define LOG_RUN_TIME // Define this if you want to keep a record of run time when in horn mode for battery usage analysis.
#define EEPROM_PIECEWISE_SIZE 81 // 1 length byte + 10 linear functions for a piecewise function
#define EEPROM_TIMER1_PIECEWISE 0x35e // Near the end so that the wear levelling can have the start
#define EEPROM_TIMER2_PIECEWISE EEPROM_TIMER1_PIECEWISE + EEPROM_PIECEWISE_SIZE
#define EEPROM_PIECEWISE_MAX_LENGTH 10 // Max number of functions piecewise function for sanity checking before allocating ram.

/**
 * @brief User interface
 * 
 */
#define LONG_PRESS_TIME 2000
#define MENU_TIMEOUT 30000

/**
 * @brief Accelerometer
 * 
 */
#define ACCEL_INSTALLED
#define ACCEL_X_PIN A4
#define ACCEL_Y_PIN A3
#define ACCEL_Z_PIN A2
#define ACCEL_POWER_PIN A5
#define ACCEL_PINS_SLEEP_MODE 0b00000000 // Input or output
#define ACCEL_PINS_SLEEP_STATE 0b11000011 // Pulled high (potentially through a resistor) or low.

/**
 * @brief Other defines
 * 
 */
#define WELCOME_MSG "Bike horn V" VERSION " started. Compiled " __TIME__ ", " __DATE__
#define MANUAL_CUTOFF // Allow notes to be stopped as we aren't using tone() to make the noises.
#define ENABLE_CALLBACKS
#define DEFAULT_TEMPO 120 // UI beeps
#define IS_PRESSED(PIN) !digitalRead(PIN) // Macro to make things a little clearer


/**
 * @brief Libraries to include
 * 
 */
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