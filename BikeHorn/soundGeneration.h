/** soundGeneration.h
 * A custom class for making noise, tuned for my specific circuit and piezo.
 * Also contains a chirp class that can be used for making chirp noises.
 * 
 * Written by Jotham Gates
 * 
 * Last modified 12/06/2021
 */

class BikeHornSound : public TimerOneSound {
    public:
        /** Stops the sound and sets the boost pwm back to idle */
        void stopSound() {
            // Shutdown timer 1
            TCCR1A = 0;
            TCCR1B = 0;
            PORTD &= ~(1 << PB1); // Set pin low just in case it is left high (not sure if needed)

            // Set timer 2 back to idle
            OCR2A = IDLE_DUTY; // Enough duty to keep the voltage up ready for next note
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

        /** Returns the value at which the pin should go low each time. Also sets the pwm duty of boost as it is called around the right time */
        uint16_t m_compareValue(uint16_t counter) {
            // For Timer 2: (May need to be adjusted / optimised by hand
            uint16_t timer2Comp;
            if(counter > 16197) {
                timer2Comp = 183;
            } else if(counter > 9089) {
                timer2Comp = 258 - counter/215;
            } else if(counter > 1011) {
                timer2Comp = 216;
            } else if(counter > 424) {
                timer2Comp = counter/9 + 107;
            } else {
                timer2Comp = 153;
            }

            // For Timer 1: (May need to be adjusted / optimised by hand
            uint16_t timer1Comp;
            if(counter > 13619) {
                timer1Comp = counter;
            } else if(counter > 1350) {
                timer1Comp = 1*counter - 1304;
            } else if(counter > 158) {
                timer1Comp = counter/12 - 1;
            } else {
                timer1Comp = 12;
            }

            // Set Timer 2 now (a bit not proper, but should work)
            OCR2A = timer2Comp;
            return timer1Comp;
        }
};

/**
 * lass to create the warbling noise
 */
class Warble {
    public:
        /**
         * Initialises the library with the given parameters
         */
        void begin(BikeHornSound *newSoundGenerator, uint16_t newLower, uint16_t newUpper, uint16_t newRiseTime, uint16_t newFallTime, uint16_t newUpdateInterval) {
            soundGenerator = newSoundGenerator;
            soundGenerator->begin();

            lower = newLower;
            upper = newUpper;
            riseTime = newRiseTime;
            fallTime = newFallTime;
            updateInterval = newUpdateInterval;
        }

        /**
         * Updates the chirp as required
         */
        void update() {
            if(isActive) {
                uint32_t time = millis();
                if(time - lastUpdate > updateInterval) {
                    lastUpdate = time;
                    // Calculate the next frequency to play
                    uint16_t newFreq;
                    if(isRising) {
                        newFreq = frequency + (updateInterval * (upper - lower)) / riseTime;
                        if(newFreq >= upper) {
                            // Time to go back down again
                            isRising = false;
                            newFreq = upper;
                        }
                    } else {
                        newFreq = frequency - (updateInterval * (upper - lower)) / fallTime;
                        if(newFreq <= lower) {
                            // Time to go back up again
                            isRising = true;
                            newFreq = lower;
                        }
                    }
                    soundGenerator->playFreq(newFreq);
                    frequency = newFreq;
                }
            }
        }

        /**
         * Starts the chirp
         */
        void start() {
            isRising = false;
            lastUpdate = millis();
            frequency = upper;
            isActive = true;
        }

        /**
         * Stops all sound
         */
        void stop() {
            soundGenerator->stopSound();
            isActive = false;
        }

    private:
        BikeHornSound *soundGenerator;
        uint16_t lower, upper, riseTime, fallTime, frequency;
        bool isRising;
        bool isActive = false;
        uint32_t lastUpdate, updateInterval;

};