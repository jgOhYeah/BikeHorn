/**
 * @file OptimiserSketch.ino
 * @author Jotham Gates
 * @brief Sketch for use with the BikeHornOptimiser tool for tuning the horn to get best performance
 * @date 2021-12-30
 * 
 * Upload this sketch to the horn and run BikeHornOptimiser.py.
 * NOTE: This script DOES NOT include any low power features. When you are finished, either remove
 * the batteries or reflash the proper BikeHorn sketch (that does have power saving features),
 * otherwise you will end up with flat batteries.
 */

#include <EEPROM.h>

#define PIEZO_PIN 9 // Fixed as PB1 (Pin 9 on Arduino Nano)
#define BOOST_PIN 11 // Fixed as PB3 (Pin 11 on Arduino Nano)
#define LED_EXTERNAL 7
#define SERIAL_BAUD 38400
#define EEPROM_TIMER1_PIECEWISE 1 // TODO: Work out bytes and addresses that don't interfere with EEPROM wear leveling 
#define EEPROM_TIMER2_PIECEWISE 82 // 81 bytes for up to 10 points

#define VERSION "0.0.0a"
#define WELCOME_MSG "Bike horn OPTIMISER V" VERSION " started. Compiled " __TIME__ ", " __DATE__

/**
 * @brief Initial setup and signalling to the computer
 * 
 */
void setup() {
    // Initial setup
    Serial.begin(SERIAL_BAUD);
    Serial.setTimeout(__LONG_MAX__);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_EXTERNAL, OUTPUT);
    pinMode(PIEZO_PIN, OUTPUT);
    pinMode(BOOST_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_EXTERNAL, HIGH);
    Serial.println(F(WELCOME_MSG));

    // Wait for the computer
    Serial.println(F("~`~`Ready`~`~"));
}

/**
 * @brief Main program loop
 * 
 */
void loop() {
    // Wait for the start of the message
    digitalWrite(LED_EXTERNAL, HIGH);
    Serial.find("~`~`");
    while(!Serial.available());
    char instruction = Serial.read();
    digitalWrite(LED_EXTERNAL, LOW);

    bool simpleAck = true;

    // Tune playing settings
    uint16_t t1Top, t1Compare;
    uint8_t t2Compare;

    // Reading and writing to eeprom
    uint16_t address;
    int16_t threshold, multiplier, divisor, constant;
    uint8_t length;

    // Interpret the instruction
    switch(instruction) {
        case 'p':
            // Play a note
            t1Top = Serial.parseInt();
            Serial.find('~');
            t1Compare = Serial.parseInt();
            Serial.find('~');
            t2Compare = Serial.parseInt();

            playNote(t1Top, t1Compare, t2Compare);
            break;
        case 's':
            // Stop all timers and noise
            stop();
            break;
        case 'l':
            // Write the length of the config to eeprom at the start
            address = Serial.parseInt();
            Serial.find('~');
            length = Serial.parseInt();
            writeStart(address, length);

        case 'w':
            // Write eeprom data point
            address = Serial.parseInt();
            Serial.find('~');
            threshold = Serial.parseInt();
            Serial.find('~');
            multiplier = Serial.parseInt();
            Serial.find('~');
            divisor = Serial.parseInt();
            Serial.find('~');
            constant = Serial.parseInt();

            writePoint(address, threshold, multiplier, divisor, constant);

        case 'r':
            // Read from eeprom
            address = Serial.parseInt();
            sendPoint(address);
            simpleAck = false;
        
        case 'm':
            // Read the length
            address = Serial.parseInt();
            sendStart(address);
            simpleAck = false;

    }

    // Wait for the end of the message
    Serial.find("`~`~");

    // Acknowledge
    if(simpleAck) {
        Serial.println(F("~`~`Ack`~`~"));
    }
}

/**
 * @brief Sets up the timers to play with the given parameters
 * 
 * @param t1Top timer 1 top, to be put in ICR1
 * @param t1Compare timer 1 compare, to be put in OCR1A
 * @param t2Compare timer 2 compare, to be put in OCR2A (top is fixed at 0xff)
 */
void playNote(uint16_t t1Top, uint16_t t1Compare, uint8_t t2Compare) {
    // Setup non inverting mode (duty cycle is sensible), fast pwm mode 14 on PB1 (Pin 9)
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS11); // With prescalar 8 (with a clock frequency of 16MHz, can get all notes required)
    ICR1 = t1Top; // Calculate the corresponding counter
    OCR1A = t1Compare; // Duty cycle

    TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20); // Mode 3, fast PWM, reset at 255
    TCCR2B = (1<< CS20); // Prescalar 1
    OCR2A = t2Compare; // Duty cycle
}

/**
 * @brief Shuts down the timers and stops making noise
 * 
 */
void stop() {
    TCCR1A = 0;
    TCCR1B = 0;
    TCCR2A = 0;
    TCCR2B = 0;

    digitalWrite(BOOST_PIN, LOW);
    digitalWrite(PIEZO_PIN, LOW);
}

/**
 * @brief Writes a data point to eeprom at a given address
 * 
 */
void writePoint(uint16_t address, int16_t threshold, int16_t multiplier, int16_t divisor, int16_t constant) {
    writeInt(address, threshold);
    writeInt(address+2, multiplier);
    writeInt(address+4, divisor);
    writeInt(address+6, constant);
}

/**
 * @brief Writes a byte to eeprom at the given address, used to indicate the length of a configuration
 * 
 */
void writeStart(uint16_t address, uint8_t length) {
    EEPROM.update(address, length);
}

/**
 * @brief Reads and sends a data point at a given address
 * 
 */
void sendPoint(uint16_t address) {
    uint16_t threshold = readInt(address);
    int16_t multiplier = readInt(address+2);
    int16_t divisor = readInt(address+4);
    int16_t constant = readInt(address+6);

    Serial.print(F("~`~`R"));
    Serial.print(threshold);
    Serial.write('~');
    Serial.print(multiplier);
    Serial.write('~');
    Serial.print(divisor);
    Serial.write('~');
    Serial.print(constant);
    Serial.println(F("`~`~"));
}

/**
 * @brief Reads the byte at a certain address in eeprom
 * 
 */
void sendStart(uint16_t address) {
    uint8_t length = EEPROM.read(address);
    Serial.print(F("~`~`R"));
    Serial.print(length);
    Serial.println(F("`~`~"));
}

/**
 * @brief Writes a 16 bit integer in little endian format to eeprom
 *
 */
void writeInt(uint16_t address, int16_t value) {
    EEPROM.update(address, value & 0xff);
    EEPROM.update(address+1, (value > 0x08) & 0xff);
}

/**
 * @brief Reads an int in little endian format from eeprom
 * 
 * @return int16_t 
 */
int16_t readInt(uint16_t address) {
    return EEPROM.read(address) | (EEPROM.read(address+1) << 8);
}