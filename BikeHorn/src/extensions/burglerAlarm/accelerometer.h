/** accelerometer.h
 * Code for using the accelerometer
 * 
 * Written by Jotham Gates
 * Last modified 23/02/2024
 */

#pragma once
#ifdef ACCEL_INSTALLED
#include "statistics.h"

// #define ACCELEROMETER_ABS_CHANGE // If defined, store and process the absolute value, otherwise the raw change that may include negatives as well
// #define ACCEL_DEBUG

/**
 * @brief Class for handling an accelerometer axis.
 * 
 */
class AccelerometerAxis {
    public:
        /**
         * @brief Construct a new Accelerometer Axis object.
         * 
         * @param channel the ADC channel on the multiplexer. `A0` is 0, `A1`
         * is `1`, ...
         */
        AccelerometerAxis(const uint8_t channel): m_channel(channel) {}

        /**
         * @brief Call this to add a value to the buffer when calculating
         * background noise.
         * 
         */
        void calibrate() {
            int16_t current = analogRead(m_channel);
            int16_t diff = current - m_previous;
#ifdef ACCELEROMETER_ABS_CHANGE
            m_stats.add(abs(diff));
#else
            m_stats.add(diff);
#endif
            m_previous = current;
        }

        /**
         * @brief Measures the accelerometer axis, calculates averages and standard deviation.
         * 
         * @return true if this is extremely unlikely that the value detected was due to noise (moved).
         * @return false if it was more likely that the value detected was due to noise (not moved).
         */
        bool isMoved() {
            // Adc parts from https://www.gammon.com.au/adc
            ADMUX = bit(REFS0) | ((m_channel-A0) & 0x07);  // AVcc, set the mux
            bitSet(ADCSRA, ADSC);  // Start a conversion

            // Time consuming operations
            double mean = m_stats.standardDev.mean();
            double std = m_stats.standardDev.std();
#ifdef ACCEL_DEBUG
            Serial.print(mean);
            Serial.write(' ');
            Serial.print(std);
            Serial.write(' ');
#endif

            // Wait until the ADC conversion is finished
            while (bit_is_set(ADCSRA, ADSC)) {
                // Can often be somewhere in the range of 0 to 50 iterations with incrementing int count
            }

            // Run the analysis
            int16_t current = ADC;
            int16_t change = current-m_previous;
#ifdef ACCELEROMETER_ABS_CHANGE
            change = abs(change);
#endif
#ifdef ACCEL_DEBUG
            Serial.print(current);
            Serial.write(' ');
            Serial.print(change);
            Serial.write(' ');
#endif
            bool result = m_stats.isUnlikely(change, mean, max(std, 1));

            // Add the results to the sample set.
            m_stats.update(change);
            m_previous = current;

            return result;
        }

    private:
        const uint8_t m_channel;
        NullHypothesis<double> m_stats;
        int16_t m_previous;
};

/**
 * @brief Class for analysing movement of an analogue, 3 channel accelerometer.
 * 
 */
class Acceleromenter {
    public:
        /**
         * @brief Turns the accelerometer on.
         * 
         */
        inline void powerOn() const {
            pinMode(ACCEL_POWER_PIN, OUTPUT);
            digitalWrite(ACCEL_POWER_PIN, HIGH);
        }

        /**
         * @brief Turns the accelerometer off.
         * 
         */
        inline void stop() const {
            pinMode(ACCEL_POWER_PIN, INPUT);
            ADCSRA = 0; // Switch of the ADC
        }

        /**
         * @brief Turns on the ADC
         * 
         */
        inline void startADC() const {
            ADCSRA =  bit (ADEN);   // turn ADC on
            ADCSRA |= bit (ADPS0) |  bit (ADPS1) | bit (ADPS2);  // Prescaler of 128
        }

        /**
         * @brief Turns the accelerometer and ADC on
         * 
         */
        inline void start() const {
            powerOn();
            startADC();
        }

        /**
         * @brief Adds a value to fill the buffer up.
         * 
         */
        inline void calibrate() {
            m_xAxis.calibrate();
            m_yAxis.calibrate();
            m_zAxis.calibrate();
        }

        /**
         * @brief Checks whether the accelerometer has been bumped.
         * 
         * @return true if it has been moved.
         * @return false if not moved.
         */
        inline bool isMoved() {
            // Need to stop any short circuiting.
            bool x = m_xAxis.isMoved();
            bool y = m_yAxis.isMoved();
            bool z = m_zAxis.isMoved();
            bool result = x || y || z;
#ifdef ACCEL_DEBUG
            Serial.println(result);
#endif
            return result;
        }

    private:
        AccelerometerAxis m_xAxis = AccelerometerAxis(ACCEL_X_PIN);
        AccelerometerAxis m_yAxis = AccelerometerAxis(ACCEL_Y_PIN);
        AccelerometerAxis m_zAxis = AccelerometerAxis(ACCEL_Z_PIN);
};
#endif