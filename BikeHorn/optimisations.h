/** optimisations.h
 * Configurations and tuning for individual horns.
 * 
 * Written by Jotham Gates.
 * Last modified 11/01/2022
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
         * {[2 bytes for threshold], [1 byte for the multiplier], [2 bytes for the divisor], [3 bytes for the constant]}
         * @endcode
         * 
         */
        void load(uint16_t address) {
            m_threshold = m_readInt16(address);
            m_multiplier = EEPROM.read(address + 2);
            m_divisor = m_readInt16(address + 3);
            m_constant = m_readInt24(address + 5);
        }

        /**
         * @brief Transforms the input to produce the result
         * 
         * @param input the input to the function
         * @return uint16_t the result
         */
        uint16_t apply(uint16_t input) {
            return ((int32_t)m_multiplier * input) / m_divisor + m_constant;
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
            return length;
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
        int16_t m_readInt16(uint16_t address) {
            return EEPROM.read(address) | (EEPROM.read(address+1) << 8);
        }
        int32_t m_readInt24(uint16_t address) {
            return EEPROM.read(address) |
                  (EEPROM.read(address+1) << 8) |
                  (EEPROM.read(address+2) << 16);
        }
        uint16_t m_threshold;
        int32_t m_constant;
        uint8_t m_multiplier;
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
 */
class PiecewiseLinear {
    public:
        /**
         * @brief Initialises and loads the parameters from eeprom.
         * 
         * @param address the address of the first byte in eeprom.
         * 
         * @returns true if the length and data is reasonable, false otherwise (potentially bad data)
         */
        bool begin(uint16_t address) {
            // Load first address, this has the number of points and allocate ram for each point
            m_length = EEPROM.read(address);
            // Sanity checking
            if(m_length != 0 && m_length <= EEPROM_PIECEWISE_MAX_LENGTH) {
                m_functions = (LinearFunction *)malloc(m_length * sizeof(LinearFunction));

                // For each address (sizeof), create a LinearFunction object and fill it out
                for(uint8_t i = 0; i < m_length; i++) {
                    m_functions[i].load(i*LinearFunction::EEPROM_BYTES + address + 1);
                }

                // Check of the thresholds are strictly increasing as another sanity check
                for(uint8_t i = 1; i < m_length; i++) {
                    if(m_functions[i].getThreshold() <= m_functions[i-1].getThreshold()) {
                        // Something is wrong with the thresholds.
                        return false;
                    }
                }
                // Successfully loaded with acceptable data
                return true;
            } else {
                // Something is wrong with the length
                return false;
            }
            
        }

        /**
         * @brief Applies the piecewise function to the input
         * 
         * @param input x in f(x)
         * @return uint16_t f(x)
         */
        uint16_t apply(uint16_t input) {
            // Linear search through linear functions to find correct one to apply (the last one where the threshold is greater >= the input)
            // See the apply method in BikeHornOptimiser.py
            // Catch not being initialised properly
            if(m_length) {
                // Apply as normal
                uint8_t i = 1;
                for(; i < m_length; i++) {
                    if(m_functions[i].underThreshold(input)) {
                        // Return previous (or else if the last one)
                        // Serial.print(F(". Choosing "));
                        // Serial.println(i-1);
                        return m_functions[i-1].apply(input);
                    }
                }

                // Serial.println(F(". Choosing last function"));
                return m_functions[m_length-1].apply(input);
            } else {
                // Return 0 as not initialised
                return 0;
            }
        }

        /**
         * @brief Prints out a representation of the function to the serial console
         * 
         */
        void print() {
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
                Serial.print(m_functions[i].getThreshold());
                Serial.write(" <= x < ");
                if(i != m_length-1) { // Cope with the last one being an else
                    Serial.print(m_functions[i+1].getThreshold());
                } else {
                    Serial.println("Inf.");
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
        uint8_t m_length = 0;
};