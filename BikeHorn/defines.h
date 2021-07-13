#define VERSION "1.0.0"

#define PIEZO_PIN 9
#define BOOST_PIN 11
#define LED_EXTERNAL 7
#define BUTTON_HORN 2
#define BUTTON_MODE 3

#define IDLE_DUTY 5 // 9.4% duty cycle, keeps the voltage up when not playing
#define MIDI_CHANNEL 0 // Zero indexed, so many software shows ch. 0 as ch. 1
#define DEBOUNCE_TIME 20

// Define this if you want to keep a record of run time when in horn mode for battery usage analysis.
#define LOG_RUN_TIME
#define LOG_VERSION 0

#define MANUAL_CUTOFF // Allow notes to be stopped.

#define WELCOME_MSG "Bike horn V" VERSION " started. Compiled " __TIME__ ", " __DATE__