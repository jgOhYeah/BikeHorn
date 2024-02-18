/** defines_nano.h
 * Pins and definitions specific to Arduino Nano (ATmega328p) based units.
 * 
 * Written by Jotham Gates.
 * Last modified 15/02/2024
 */
#pragma once

/**
 * @brief Pins and system setup
 * 
 */
#define PIEZO_PIN 9 // Fixed as PB1 (Pin 9 on Arduino Nano)
#define BOOST_PIN 11 // Fixed as PB3 (Pin 11 on Arduino Nano)
#define LED_EXTERNAL 7
#define BUTTON_HORN 2 // Needs to be interrupt capable (pin 2 or 3)
#define BUTTON_MODE 3 // Needs to be interrupt capable (pin 2 or 3)

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

#define CPU_TYPE_MSG "ATmega328p"
#define USES_LED_BUILTIN