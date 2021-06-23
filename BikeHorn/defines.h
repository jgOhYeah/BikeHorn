#define VERSION "0.9"

#define PIEZO_PIN 9
#define BOOST_PIN 11
#define LED_EXTERNAL 7
#define BUTTON_HORN 2
#define BUTTON_MODE 3

#define IDLE_DUTY 5 // 9.4% duty cycle, keeps the voltage up when not playing

#define MIDI_CHANNEL 0 // Zero indexed, so many software shows ch. 0 as ch. 1

// #define DEBUG_ENABLE
#define DEBOUNCE_TIME 20

#define WELCOME_MSG "Bike horn V" VERSION " started. Compiled " __TIME__ ", " __DATE__

// Defines to make printing debug messages easier, as well as making it easier to disable them later.
#ifdef DEBUG_ENABLE
  #define STR_HELPER(x) #x
  #define STR(x) STR_HELPER(x)
  #define INFO_STRING __FILE__ ", Line " STR(__LINE__) ":\t"
  #define ERROR_STRING "Error: " INFO_STRING
  #define DIALOG_STRING "Dialog: " INFO_STRING
  #define DEBUG_E(MESSAGE) Serial.print(F(ERROR_STRING)); Serial.println(MESSAGE)
  #define DEBUG_D(MESSAGE) Serial.print(F(DIALOG_STRING)); Serial.println(MESSAGE)
  #define DEBUG_VALUE(LABEL,VALUE) Serial.print('\t'); Serial.print(LABEL); Serial.print(F(":\t")); Serial.println(VALUE)
  #define DEBUG_VALUE_B(LABEL,VALUE) Serial.print('\t'); Serial.print(LABEL); Serial.print(F(":\t0b")); Serial.println(VALUE,BIN)
  #define DEBUG_VALUE_H(LABEL,VALUE) Serial.print('\t'); Serial.print(LABEL); Serial.print(F(":\t0x")); Serial.println(VALUE,HEX)
#else
  #define DEBUG_E(MESSAGE)
  #define DEBUG_D(MESSAGE)
  #define DEBUG_VALUE(LABEL,VALUE)
  #define DEBUG_VALUE_B(LABEL,VALUE)
  #define DEBUG_VALUE_H(LABEL,VALUE)
#endif
