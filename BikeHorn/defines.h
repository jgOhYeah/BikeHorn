/** defines.h
 * Definitions and settings for the BikeHorn.
 * 
 * For more details, see README.md or go to
 * https://github.com/jgOhYeah/BikeHorn
 * 
 * Written by Jotham Gates
 * Last modified 15/07/2021
 */
 
#define VERSION "1.0.1"

#define PIEZO_PIN 9 // Fixed as PB1 (Pin 9 on Arduino Nano)
#define BOOST_PIN 11 // Fixed as PB3 (Pin 11 on Arduino Nano)
#define LED_EXTERNAL 7
#define BUTTON_HORN 2 // Needs to be interrupt capable (pin 2 or 3)
#define BUTTON_MODE 3 // Needs to be interrupt capable (pin 2 or 3)

#define IDLE_DUTY 5 // 9.4% duty cycle, keeps the voltage up when not playing
#define MIDI_CHANNEL 0 // Zero indexed, so many software shows ch. 0 as ch. 1
#define DEBOUNCE_TIME 20

// Define this if you want to keep a record of run time when in horn mode for battery usage analysis.
#define LOG_RUN_TIME
#define LOG_VERSION 1

#define MANUAL_CUTOFF // Allow notes to be stopped.

#define WELCOME_MSG "Bike horn V" VERSION " started. Compiled " __TIME__ ", " __DATE__