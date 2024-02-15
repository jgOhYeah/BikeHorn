/** statistics.h
 * Statistics used for movement detection.
 * 
 * Written by Jotham Gates
 * Last modified 22/12/2022
 */

#pragma once
#include <cppQueue.h>

/**
 * @brief Class for calculating incremental standard deviations.
 * Based off the incremental algorithm in:
 * https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Computing_shifted_data
 * 
 * This uses doubles. Some quick testing showed that doubles are on par with or
 * slightly faster than using uint32_t types instead. This is probably because
 * of the large division and square root finding required.
 */
class IncrementalStdDev {
    public:
        /**
         * @brief Returns the standard deviation.
         * 
         * @return double 
         */
        double std() {
            return sqrt(variance());
        }

        /**
         * @brief Returns the variance.
         * 
         * @return double 
         */
        double variance() {
            return (Ex2 - (Ex*Ex)/n) / (n - 1);
        }

        /**
         * @brief Calculates the mean.
         * 
         * @return double 
         */
        double mean() {
            return K + Ex / n;

        }

        /**
         * @brief Adds a value to the set of numbers.
         * 
         * @param x 
         */
        void add(double x) {
            if (n == 0) {
                K = x;
            }
            n++;
            double xmk = x - K;
            Ex += xmk;
            Ex2 += xmk * xmk;
        }

        /**
         * @brief Removes a variable from the set of numbers.
         * 
         * @param x 
         */
        void remove(double x) {
            n--;
            double xmk = x - K;
            Ex -= xmk;
            Ex2 -= xmk * xmk;
        }

        /**
         * @brief Resets everything back to empty.
         */
        void clear() {
            n = 0;
            K = 0;
            Ex = 0;
            Ex2 = 0;
        }

        uint8_t n = 0;

    private:
        double K = 0;
        double Ex = 0;
        double Ex2 = 0;
};

/**
 * @brief Class for managing and testing the null hypothesis.
 * 
 * This checks if it is likely / not unreasonable for a value to occur if there
 * is no change.
 * 
 * @tparam DataType is the data type to use for testing. This must support
 * negative numbers (can't be unsigned).
 */
template <typename DataType>
class NullHypothesis {
    public:
        /**
         * @brief Checks if the given value is likely to occur without some
         * change to the system.
         * 
         * Calculates the mean and standard deviation as part of this (may take
         * time).
         * 
         * @param value the value to compare to the saved samples.
         * @return true if the value is unlikely to occur.
         * @return false if the value could feasably be noise.
         */
        bool isUnlikely(DataType value) {
            return isUnlikely(value, standardDev.mean(), standardDev.std());
        }

        /**
         * @brief Checks if the given value is likely to occur without some
         * change to the system.
         * 
         * Uses the provided means and standard deviation.
         * 
         * @param value the value to compare to the saved samples.
         * @param mean the mean to use.
         * @param stdDev the standard deviation to use.
         * @return true if the value is unlikely to occur.
         * @return false if the value could feasably be noise.
         */
        static bool isUnlikely(DataType value, DataType mean, DataType stdDev) {
            // Based on the last x readings, is it likely that this value is
            // from the same set?
            DataType difference = value - mean;
            return abs(difference) > STD_DEVIATIONS*stdDev;
        }

        /**
         * @brief Adds a new sample and removes the oldest.
         * 
         * @param value is the new value to add.
         */
        void update(DataType value) {
            // Remove the old value from the queue and standard deviation.
            DataType old;
            m_queue.pop(&old);
            standardDev.remove(old);

            // Add the new value
            m_queue.push(&value);
            standardDev.add(value);
        }

        /**
         * @brief Adds a new value without removing the oldest from the set of
         * samples.
         * 
         * @param value is the value to add.
         */
        void add(DataType value) {
            m_queue.push(&value);
            standardDev.add(value);
        }
    
        /**
         * @brief Object that calculates the standard deviation and mean.
         * 
         */
        IncrementalStdDev standardDev;

    private:
        cppQueue m_queue = cppQueue(sizeof(DataType), PREVIOUS_RECORDS, FIFO, true);
};