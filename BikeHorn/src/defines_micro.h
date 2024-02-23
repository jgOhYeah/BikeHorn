/** defines_micro.h
 * Pins and definitions specific to Arduino Micro (ATmega32u4) based units.
 * 
 * Written by Jotham Gates and Nguyen Nguyen.
 * Last modified 23/02/2024
 */
#pragma once

/**
 * @brief Pins and system setup
 * 
 */
#define PIEZO_PIN 9 // Fixed as PB5 - OC1A (Pin D9 on Arduino Micro)
#define BOOST_PIN 6 // Fixed as PD7 - OC4D, (Pin D6 on Arduino Micro). Was originally PE6.
#define LED_EXTERNAL 10 // PB6
#define BUTTON_HORN 5 // Ideally interrupt capable but not, D5, PC6,
#define BUTTON_MODE 13 // Ideally interrupt capable but not, D13, PC7.

#define IDLE_DUTY 1 // /255 duty cycle, keeps the voltage up when not playing

/**
 * @brief Accelerometer
 * 
 */
// #define ACCEL_INSTALLED
// #define ACCEL_X_PIN A4
// #define ACCEL_Y_PIN A3
// #define ACCEL_Z_PIN A2
// #define ACCEL_POWER_PIN A5
// #define ACCEL_PINS_SLEEP_MODE 0b00000000 // Input or output
// #define ACCEL_PINS_SLEEP_STATE 0b11000011 // Pulled high (potentially through a resistor) or low.

#define CPU_TYPE_MSG "ATmega32u4"
#define NO_BUTTON_INTERRUPTS