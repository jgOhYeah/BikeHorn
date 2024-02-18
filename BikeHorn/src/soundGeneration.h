/** soundGeneration.h
 * A custom class for making noise, tuned for my specific circuit and piezo.
 * Also contains a chirp class that can be used for making chirp noises.
 * 
 * Written by Jotham Gates
 * 
 * Last modified 07/11/2022
 */
#pragma once

/**
 * @brief Handles the task of making the most noise possible.
 * 
 */
class BikeHornSound : public TimerOneSound {
    public:
        void begin() {
            // Load and print configs
            if(m_timer1Piecewise.begin(EEPROM_TIMER1_PIECEWISE) && m_timer2Piecewise.begin(EEPROM_TIMER2_PIECEWISE)) {
                Serial.println(F("Timer 1 Optimisation settings:"));
                m_timer1Piecewise.print();
                Serial.println();
                Serial.println(F("Timer 2 Optimisation settings:"));
                m_timer2Piecewise.print();
            } else {
                // There was an issue initialising the piecewise functions
                Serial.println(F("ERROR: At least 1 piecewise function for optimising volume was a bit suspect and could not be loaded from EEPROM.\r\nAre you sure you have uploaded the optimised functions to EEPROM and the addresses are correct?\r\nSee https://github.com/jgOhYeah/BikeHorn/tree/main/Tuning for more info."));
            }
        }

        /** Stops the sound and sets the boost pwm back to idle */
        void stopSound() {
            TIMSK1 = 0; // Disable the interrupt for changing the piezo frequency (warble mode).

            TimerOneSound::stopSound();

            // Set boost back to idle.
            SET_IDLE_DUTY();
        }

        /**
         * Plays a note of a given frequency.
         * This should be at least 31Hz
         */
        void playFreq(uint16_t frequency) {
            // Setup non inverting mode (duty cycle is sensible), fast pwm mode 14 on PB1 (Pin 9)
            TCCR1A = (1 << COM1A1) | (1 << WGM11);
            TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS11); // With prescalar 8 (with a clock frequency of 16MHz, can get all notes required)
            ICR1 = F_CPU / 8 / frequency; // Calculate the corresponding counter
            OCR1A = m_compareValue(ICR1); // Duty cycle
        }

        /**
         * Sets the variables to do software double buffering that will update the values at the correct part of the cycle to stop glitches.
         */
        void changeFreq(uint16_t frequency) {
            TIMSK1 = 0; // Disable this interrupt while updating the values
            nextTop = F_CPU / 8 / frequency;
            nextComp = m_compareValue(nextTop);
            TIMSK1 = (1 << TOIE1); // Enable interrupts when overflowing
        }

        static volatile uint16_t nextTop;
        static volatile uint16_t nextComp;

    private:
        /** Returns the value at which the pin should go low each time. Also sets the pwm duty of boost as it is called around the right time */
        uint16_t m_compareValue(uint16_t counter) {
#ifdef __AVR_ATmega32U4__
            uint8_t boostSetting = m_timer2Piecewise.apply(counter);
            TC4H = 0x0;
            OCR4D = boostSetting;
#else
            // Set Timer 2 now (a bit not proper, but should work)
            OCR2A = m_timer2Piecewise.apply(counter);
#endif
        return m_timer1Piecewise.apply(counter);
        }

        PiecewiseLinear m_timer1Piecewise;
        PiecewiseLinear m_timer2Piecewise;
};

#ifdef ENABLE_WARBLE
/**
 * Class to create the warbling noise
 */
class Warble {
    public:
        /**
         * Initialises the library with the given parameters
         */
        void begin(BikeHornSound *newSoundGenerator, uint16_t newLower, uint16_t newUpper, uint32_t newRiseTime, uint32_t newFallTime) {
            m_soundGenerator = newSoundGenerator;
            // m_soundGenerator->begin();

            m_lower = newLower;
            m_upper = newUpper;
            m_riseTime = newRiseTime;
            m_fallTime = newFallTime;
        }

        /**
         * Updates the chirp as required
         */
        void update() {
            if(m_isActive) {
                uint32_t time = micros();
                if(time - m_lastUpdate > m_updateInterval) {
                    m_lastUpdate = time;
                    if(m_isRising) {
                        if(m_frequency != m_upper) {
                            // Keep going up
                            m_frequency+=WARBLE_STEP;
                        } else {
                            // Swap to falling
                            m_isRising = false;
                            m_updateInterval = m_timeStep(m_fallTime);
                        }
                    } else {
                        if(m_frequency != m_lower) {
                            // Keep going down
                            m_frequency-=WARBLE_STEP;
                        } else {
                            // Swap to rising
                            m_isRising = true;
                            m_updateInterval = m_timeStep(m_riseTime);
                        }
                    }
                    m_soundGenerator->changeFreq(m_frequency);
                }
            }
        }

        /**
         * Starts the chirp
         */
        void start() {
            m_isRising = false;
            m_lastUpdate = micros();
            m_frequency = m_upper;
            m_isActive = true;
            m_updateInterval = m_timeStep(m_fallTime);
            m_soundGenerator->playFreq(m_frequency);
        }

        /**
         * Stops all sound
         */
        void stop() {
            m_soundGenerator->stopSound();
            m_isActive = false;
        }

    private:
        BikeHornSound *m_soundGenerator;
        uint16_t m_lower, m_upper, m_frequency;
        bool m_isRising;
        bool m_isActive = false;
        uint32_t m_lastUpdate, m_updateInterval, m_riseTime, m_fallTime;

        uint32_t m_timeStep(uint32_t sweepTime) {
            return WARBLE_STEP * sweepTime / (m_upper - m_lower);
        }

};

#endif