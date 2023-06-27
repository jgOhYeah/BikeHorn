/** measureBattery.h
 * Prints the battery voltage to the console every so often.
 * 
 * Written by Jotham Gates
 * Created 09/07/2022
 * Last modified 27/06/2023
 */
#pragma once
#include "extensionsManager.h"

class MeasureBatteryExtension: public Extension {
    public:
        void onStart() {
            printUpdate();
        }

        void onWake() {
            printUpdate();
        }

        void onSleep() {
            printUpdate();
        }


    private:
        void printUpdate() {
            Serial.print(F("Battery voltage: "));
            Serial.print(readVcc());
            Serial.println(F("mv"));
        }

        /**
         * @brief Reads the supply voltage to the microcontroller and returns
         * it in millivolts.
         * 
         * Based off https://www.instructables.com/id/Secret-Arduino-Voltmeter/
         * @return uint32_t the voltage in mv.
         */
        uint32_t readVcc() {
            ADCSRA =  bit (ADEN);   // turn ADC on
            ADCSRA |= bit (ADPS0) |  bit (ADPS1) | bit (ADPS2);  // Prescaler of 128

            // Read 1.1V reference against AVcc
            // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
                ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
                ADMUX = _BV(MUX5) | _BV(MUX0) ;
#else
                ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif  
            
            delay(2); // Wait for Vref to settle
            ADCSRA |= _BV(ADSC); // Start conversion
            while (bit_is_set(ADCSRA,ADSC)); // measuring
            
            uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
            uint8_t high = ADCH; // unlocks both
            
            uint32_t result = (high<<8) | low;
            
            result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
            return result; // Vcc in millivolts
        }
};