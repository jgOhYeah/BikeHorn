/** burglerAlarm.h
 * Requires an accelerometer by installed.
 * When activated, monitors for movement and sets off the siren if too much
 * movement is detected.
 * 
 * Written by Jotham Gates
 * Last modified 01/07/2022
 */

/**
 * @brief Alarm mode
 */
void burglerAlarmMode() {
    // TODO: Implement
    /*
    recharge_time = 60 seconds
    max_count = 3
    Movement count = max_count
    while true:
        Turn accelerometer on
        Take reading
        Use fancy algorithm to compare reading with previous and check if moved significantly.
        If moved significantly:
            decrement movement count
            if count > 0:
                Set off warning beep
            else:
                Go into alarm mode
    */

}

void burglerAlarmSound() {
    // TODO: Implement
    /*
    Sound
    if 2 minutes up or key code entered correctly, go back to burgler alarm mode.*/
}
#include "extensionsManager.h"

#define SLEEP_SKIP 255

class BurglerAlarmExtension : public Extension {
    public:

        void monitor() {
            // LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
            AlarmState state = Init;
            uint16_t iterationsRemaining = 40;
            while (true) {
                switch(state) {
                    case Init:
                        // Fill up the buffers and go to Low Power
                        takeReading();
                        sleepTime = SLEEP_60MS;
                        iterationsRemaining--;
                        if(!iterationsRemaining) {
                            state = Sleep;
                        }
                        break;
                    
                    case Sleep:
                        // Long sleeps between checks, go to Awake when detected.
                        // TODO: GPIO
                        if(takeReading()) {
                            state = Awake;
                            iterationsRemaining = 40;
                        }
                        sleepTime = SLEEP_4S;
                        break;

                    case Awake:
                        // Shorter sleeps between checks, keep accelerometer on. Beep and go to Alert when detected.
                        sleepTime = SLEEP_120MS;

                        iterationsRemaining--; // Nothing happened for a while
                        if(!iterationsRemaining) {
                            state = Sleep;
                        }
                        if(takeReading()) {
                            state = Alert;
                            iterationsRemaining = 40;
                        }
                        break;
                    
                    case Alert:
                        // Same as awake, go to Countdown when detected.
                        sleepTime = SLEEP_120MS;
                        iterationsRemaining--; // Nothing happened for a while
                        if(!iterationsRemaining) {
                            state = Sleep;
                        }
                        if(takeReading()) {
                            state = Countdown;
                        }
                        break;
                    
                    case Countdown:
                        // Give 10s to enter code. If successful, return. Else go to siren.
                        // TODO:
                        state = Siren;
                        break;

                    default: // Siren
                        // Noise. If button is pressed, go to countdown while still making noise.
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
        enum AlarmState {Init, Sleep, Awake, Alert, Countdown, Siren};

};

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
        CodeEntry(const uint16_t code) : m_code(code) {}

        /**
         * @brief Resets code entry so that a new attempt at the code can be entered.
         * 
         */
        void start() {
            m_charsLeft = m_code & 0xf;
            m_inputted = 0;
            m_lastPressed = none;
            m_lastPressTime = millis();
        }

        /**
         * @brief Adds a character to the code.
         * 
         * @param character true for 1, 0 for false
         * @return true if enough characters have been obtained.
         * @return false if not enough characters have been obtained.
         */
        bool add(bool character) {
            if(m_charsLeft != 0) {
                m_inputted |= character << 15;
                m_inputted >>= 1;
                m_charsLeft--;
                return m_charsLeft == 0;
            }
        }
        
        bool update() {
            uint32_t curTime = millis();
            if(curTime - m_lastPressTime > DEBOUNCE_TIME) {
                // It is safe to assume a change now means a button has been pressed or released.
                WakePin curButton; // TODO: Rename to somethign more generic.
                if (IS_PRESSED(BUTTON_HORN)) {
                    curButton = horn;
                } else if(IS_PRESSED(BUTTON_MODE)) {
                    curButton = mode;
                } else {
                    curButton = none;
                }

                if (curButton != m_lastPressed) {
                    m_lastPressTime = curTime;
                } // TODO: Add
            } else {
                // TODO: Not update
            }
        }

        /**
         * @brief Returns true if the inputted code matches the given code,
         * otherwise false.
         * 
         */
        bool check() {
            return m_code & ~0xf == m_inputted;
        }

    private:
        const uint16_t m_code;
        int8_t m_charsLeft;
        uint16_t m_inputted;
        WakePin m_lastPressed;
        uint32_t m_lastPressTime;
};