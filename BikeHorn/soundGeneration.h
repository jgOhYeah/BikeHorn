/** soundGeneration.h
 * A custom class for making noise, tuned for my specific circuit and piezo.
 * Also contains a chirp class that can be used for making chirp noises.
 * 
 * Written by Jotham Gates
 * 
 * Last modified 01/10/2021
 */

class BikeHornSound : public TimerOneSound {
    public:
        /** Stops the sound and sets the boost pwm back to idle */
        void stopSound() {
            TIMSK1 = 0; // Disable the interrupt for changing the piezo frequency (warble mode).

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

// TODO: Put this as a static method in the sound generator class to be not so ugly.
volatile uint16_t BikeHornSound::nextTop;
volatile uint16_t BikeHornSound::nextComp;

/**
 * Interrupt for timer overflow to change the frequency of the warble safely at the correct time in the cycle without
 * using a double buffered register. The manual suggests OCR1A should be set as top as it is double buffered, however
 * this will disable PWM on the pin I am using.
 * 
 * The risk of changing ICR1 randomly at any time is that a reset of the counter only happens when the counter equals
 * ICR1. This means there is a chance that it could be changed to a value that the counter is currently above. The
 * counter would then have to count to 0xFFFF and overflow to get back to normal operation, causing a noticable glitch
 * that makes you question the reliability of the horn.
 * 
 * This ISR will only change ICR1 as it is resetting, hopefully avoiding this issue (did someone mention software
 * solution to hardware problem?)
 */
ISR(TIMER1_OVF_vect) {
    ICR1 = BikeHornSound::nextTop;
    OCR1A = BikeHornSound::nextComp;
    TIMSK1 = 0; // Disable timer 1 interrupts
}

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
            soundGenerator = newSoundGenerator;
            soundGenerator->begin();

            lower = newLower;
            upper = newUpper;
            riseTime = newRiseTime;
            fallTime = newFallTime;
        }

        /**
         * Updates the chirp as required
         */
        void update() {
            if(isActive) {
                uint32_t time = micros();
                if(time - lastUpdate > updateInterval) {
                    lastUpdate = time;
                    if(isRising) {
                        if(frequency != upper) {
                            // Keep going up
                            frequency++;
                        } else {
                            // Swap to falling
                            isRising = false;
                            updateInterval = timeStep(fallTime);
                        }
                    } else {
                        if(frequency != lower) {
                            // Keep going down
                            frequency--;
                        } else {
                            // Swap to rising
                            isRising = true;
                            updateInterval = timeStep(riseTime);
                        }
                    }
                    soundGenerator->changeFreq(frequency);
                }
            }
        }

        /**
         * Starts the chirp
         */
        void start() {
            isRising = false;
            lastUpdate = micros();
            frequency = upper;
            isActive = true;
            updateInterval = timeStep(fallTime);
            soundGenerator->playFreq(frequency);
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
        uint16_t lower, upper, frequency;
        bool isRising;
        bool isActive = false;
        uint32_t lastUpdate, updateInterval, riseTime, fallTime;

        uint32_t timeStep(uint32_t sweepTime) {
            return sweepTime / (upper - lower);
        }

};

#endif