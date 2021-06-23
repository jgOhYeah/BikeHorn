/** soundGeneration.h
 * A custom class for making noise, tuned for my specific circuit and piezo
 * 
 * Written by Jotham Gates, 12/06/2021
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