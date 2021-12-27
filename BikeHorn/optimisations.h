/** optimisations.h
 * Configurations and tuning for individual horns.
 * 
 * Written by Jotham Gates.
 * Last modified 27/12/2021
 */

/**
 * @brief Class for a single linear function as part of a piecewise function (Used for mapping duty
 * cycles for timer 1 and timer 2)
 * 
 * Functions are in the form multiplier*x/divisor + constant
 */
class LinearFunction {
    public:
        /**
         * @brief Loads the parameters for the function from an address in EEPROM.
         * 
         * @param address The starting address. LinearFunction::EEPROM_BYTES contains the number of bytes that will be read.
         * 
         * Parameters are encoded in EEPROM from address in little endian format. i.e.
         * @code {.c++}
         * {[2 bytes for threshold], [2 bytes for the multiplier], [2 bytes for the divisor], [2 bytes for the constant]}
         * @endcode
         * 
         */
        void load(uint16_t address) {
            m_threshold = m_readInt(address);
            m_multiplier = m_readInt(address + 2);
            m_divisor = m_readInt(address + 4);
            m_constant = m_readInt(address + 6);
        }

        /**
         * @brief Transforms the input to produce the result
         * 
         * @param input the input to the function
         * @return uint16_t the result
         */
        uint16_t apply(uint16_t input) {
            return m_multiplier * input / m_divisor + m_constant;
        }

        /**
         * @brief Prints the function to the serial terminal
         * 
         * @return the number of chars written
         */
        uint8_t print() {
            uint8_t length = 0;
            length += Serial.write('(');
            length += Serial.print(m_multiplier);
            length += Serial.write(")*x/(");
            length += Serial.print(m_divisor);
            length += Serial.write(") + (");
            length += Serial.print(m_constant);
            length += Serial.write(")");
        }
        
        /**
         * @brief Checks if a given number is less than the threshold
         * 
         * @param input the number to check
         * @return true if input < threshold, false otherwise
         */
        bool underThreshold(int16_t input) {
            return input < m_threshold;
        }

        /**
         * @brief Get the Threshold
         * 
         * @return uint16_t 
         */
        uint16_t getThreshold() {
            return m_threshold;
        }

        static const int EEPROM_BYTES = 8;
    private:
        /**
         * @brief Reads an integer from eeprom (little endian - lowest byte first)
         * 
         * @param address the address of the first byte to read
         * @return int16_t the value at that address
         */
        int16_t m_readInt(uint16_t address) {
            return EEPROM.read(address) | (EEPROM.read(address+1) << 8);
        }

        uint16_t m_threshold;
        int16_t m_constant;
        int16_t m_multiplier;
        int16_t m_divisor;
};

/**
 * @brief Class for a piecewise function consiting of many linear functions.
 * 
 * The parameters are loaded from EEPROM memory. The first byte is the number of linear functions,
 * followed by that number of linear functions as described above. It is assumed that the smallest
 * thresholds will be first, i.e.
 * @code {.language-id}
 * {[1 byte for number of functions], [8 bytes for function 1], [8 bytes for function 2], ...}
 * @endcode
 * 
 * 
 * For example, the code:
 * @code {.c++}
 * uint16_t result;
 * uint16_t input;
 * if(input < 12) {
 *   result = 4/5*input + 7;
 * } elseif(input < 15) {
 *   result = 5*input - 5;
 * } else {
 *   result = 3/2*input;
 * }
 * @endcode
 * would be represented as:
 * @code {.c++}
 * {4, [length]
 *  [Function 1]
 *  12, 0, [threshold]
 *  4, 0, [multiplier]
 *  5, 0, [divisor]
 *  7, 0, [constant]
 *  [Function 2]
 *  15, 0, [threshold]
 *  5, 0, [multiplier]
 *  1, 0, [divisor]
 *  -5, 0, [constant, whatever this is as a 16 bit two's complement int]
 *  [Function 3]
 *  ##, ##, [threshold, ignored]
 *  3, 0, [multiplier]
 *  2, 0, [divisor]
 *  0, 0, [constant]
 * }
 * @endcode
 * 
 */
class PiecewiseLinear {
    public:
        /**
         * @brief Initialises and loads the parameters from eeprom.
         * 
         * @param address the address of the first byte in eeprom.
         */
        void begin(uint16_t address) {
            // Load first address, this has the number of points and allocate ram for each point
            m_length = EEPROM.read(address);
            m_functions = (LinearFunction *)malloc(m_length * sizeof(LinearFunction));

            // For each address (sizeof), create a LinearFunction object and fill it out
            for(uint16_t i = 0; i < m_length; i++) {
                m_functions[i].load(i*LinearFunction::EEPROM_BYTES + address + 1);
            }
        }

        /**
         * @brief Applies the piecewise function to the input
         * 
         * @param input x in f(x)
         * @return uint16_t f(x)
         */
        uint16_t apply(uint16_t input) {
            // Linear search through linear functions to find correct one to apply (the last one where the threshold is greater than the input)
            // Threshold of last is ignored as it is assumed it is the else clause
            uint8_t i = 0;
            for(; i < m_length; i++) {
                if(!m_functions[i].underThreshold(input) | i == m_length-1) {
                    // Return previous (or else if the last one)
                    if(i!=0) {
                        // Return i-1
                        i--;
                    } else {
                        // Return the else
                        i = m_length - 1;
                    }
                    break;
                }
            }

            // Call apply of function and return
            return m_functions[i].apply(input);
        }

        /**
         * @brief Prints out a representation of the function to the serial console
         * 
         */
        void print() {
            uint16_t previousThreshold = 0;
            for(uint8_t i = 0; i < m_length; i++) {
                // Piecewise brackets
                if(i!=0) {
                    Serial.write("    ");
                } else {
                    Serial.write("fx)=");
                }
                Serial.write(" {");

                // Formula part
                const uint8_t FORMULA_WIDTH = 30;
                m_printSpaces(FORMULA_WIDTH - m_functions[i].print());

                // Range at end
                Serial.write(", ");
                Serial.print(previousThreshold);
                Serial.write(" <= x <= ");
                if(i != m_length-1) { // Cope with the last one being an else
                    Serial.print(m_functions[i].getThreshold());
                } else {
                    Serial.println(65535);
                }
                Serial.println();
            }
        }
        
        /**
         * @brief Frees any allocated memory (probably not that useful from an Arduino perspective)
         */
        void end() {
            free(m_functions);
        }
    private:
        /**
         * @brief Prints the given number of spaces.
         * 
         * @param count the number of spaces to print.
         */
        void m_printSpaces(uint8_t count) {
            for(uint8_t i = 0; i < count; i++) {
                Serial.write(' ');
            }
        }
        LinearFunction *m_functions;
        uint8_t m_length;
};