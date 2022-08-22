/** burglerAlarm.h
 * Requires an accelerometer by installed.
 * When activated, monitors for movement and sets off the siren if too much
 * movement is detected.
 * 
 * Written by Jotham Gates
 * Last modified 22/08/2022
 */

#include "extensionsManager.h"

#define SLEEP_SKIP 255
#define ENCODE_CODE(CODE, LENGTH) (CODE<<4 | LENGTH)

#define MY_CODE ENCODE_CODE(0b1001011, 7)

class BurglerAlarmExtension : public Extension {
    public:

        void monitor() {
            // LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
            AlarmState state = INIT;
            uint16_t iterationsRemaining = 40;
            uint32_t startTime;
            CodeEntry codeEntry(MY_CODE);
            while (true) {
                switch(state) {
                    case INIT:
                        // Fill up the buffers and go to Low Power
                        takeReading();
                        sleepTime = SLEEP_60MS;
                        iterationsRemaining--;
                        if(!iterationsRemaining) {
                            state = SLEEP;
                        }
                        break;
                    
                    case SLEEP:
                        // Long sleeps between checks, go to AWAKE when detected.
                        // TODO: GPIO
                        if(takeReading()) {
                            state = AWAKE;
                            iterationsRemaining = 40;
                        }
                        sleepTime = SLEEP_4S;
                        break;

                    case AWAKE:
                        // Shorter sleeps between checks, keep accelerometer on. Beep and go to ALERT when detected.
                        sleepTime = SLEEP_120MS;

                        iterationsRemaining--; // Nothing happened for a while
                        if(!iterationsRemaining) {
                            state = SLEEP;
                        }
                        if(takeReading()) {
                            state = ALERT;
                            iterationsRemaining = 40;
                        }
                        break;
                    
                    case ALERT:
                        // Same as awake, go to COUNTDOWN when detected.
                        sleepTime = SLEEP_120MS;
                        iterationsRemaining--; // Nothing happened for a while
                        if(!iterationsRemaining) {
                            state = SLEEP;
                        }
                        if(takeReading()) {
                            state = COUNTDOWN_ENTRY;
                        }
                        break;

                    case COUNTDOWN_ENTRY:
                        // Code that runs at the start of countdown
                        state = COUNTDOWN;
                        startTime = millis();
                        iterationsRemaining = 10;
                        codeEntry.start();
                        break;

                    case COUNTDOWN:
                        // Give 10s to enter code. If successful, return. Else go to siren.
                        if(curTune - startTime >= 1000) {
                            // Once per second
                            iterationsRemaining--;
                            startTime = curTune;
                        }
                        if(!iterationsRemaining) {
                            // Run out of time with no success.
                            state = SIREN;
                        }
                        break;

                    default: // SIREN
                        // Noise. If button is pressed, go to countdown while still making noise.
                        // TODO
                        Serial.println("Siren");
                }
                if (sleepTime != SLEEP_SKIP) {
                    LowPower.powerDown(sleepTime, ADC_OFF, BOD_OFF); // The horn will spend most of its life here
                }
            }
        }

        bool takeReading() {
            return false; // TODO
        }

    private:
        /**
         * @brief What state the alarm is in.
         * 
         */
        enum AlarmState {INIT, SLEEP, AWAKE, ALERT, COUNTDOWN_ENTRY, COUNTDOWN, SIREN};

};

/**
 * @brief Class for entering a pin code.
 * 
 */
class CodeEntry {
    public:
        /**
         * @brief Construct a new Code Entry object.
         * 
         * The code is formatted as a 16 bit binary number. The lowest 4 bits store the number of
         * characters in the code. The remaining upper 12 bits store the characters (0 is the mode
         * button being pressed, 1 is horn button being pressed).
         * 
         * @param code the code to accept as a 16 bit binary number.
         */
        CodeEntry(const uint16_t code) : m_code(code) {
            start();
        }

        /**
         * @brief Construct a new Code Entry object.
         * 
         * The code is formatted as a 16 bit binary number. The lowest 4 bits store the number of
         * characters in the code. The remaining upper 12 bits store the characters (0 is the mode
         * button being pressed, 1 is horn button being pressed).
         * 
         * @param pin the pin to accept (%1010 is horn, mode, horn, mode).
         * @param length the number of digits.
         */
        CodeEntry(const uint16_t pin, const uint8_t length) : m_code(ENCODE_CODE(pin, length)) {
            start();
        }

        /**
         * @brief Resets code entry so that a new attempt at the code can be entered.
         * 
         */
        void start() {
            m_charsLeft = m_code & 0xf;
            m_inputted = 0;
            m_lastPressedTime = millis();
        }

        /**
         * @brief Adds a character to the code.
         * 
         * @param character true for 1, 0 for false
         * @return true if enough characters have been obtained.
         * @return false if not enough characters have been obtained.
         */
        bool add(bool character) {
            m_charsLeft--;
            if(m_charsLeft >= 0) {
                m_inputted |= character << m_charsLeft;
                return m_charsLeft == 0;
            }
            return true;
        }
        
        /**
         * @brief Checks the buttons and inputs characters (horn is true, mode is false).
         * 
         * @return true if enough characters have been obtained.
         * @return false is not enough characters have been obtained.
         */
        bool update() {
            uint32_t curTime = millis();
            if(curTime - m_lastPressedTime > DEBOUNCE_TIME) {
                // A Button hasn't been pressed for a while.
                if (IS_PRESSED(BUTTON_HORN)) {
                    m_lastPressedTime = curTime;
                    return add(true);
                } else if(IS_PRESSED(BUTTON_MODE)) {
                    m_lastPressedTime = curTime;
                    return add(false);
                }

            } else if(IS_PRESSED(BUTTON_MODE) || IS_PRESSED(BUTTON_HORN)) {
                // A button is currently pressed
                m_lastPressedTime = curTime;
            }
            return false;
        }

        /**
         * @brief Returns true if the inputted code matches the given code,
         * otherwise false.
         * 
         */
        bool check() {
            return (m_code & ~0xf) == (m_inputted << 4);
        }

    private:
        const uint16_t m_code;
        int8_t m_charsLeft;
        uint16_t m_inputted;
        uint32_t m_lastPressedTime;
};