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

#define PIEZO_PIN 9 // Fixed as PB1 (Pin 9 on Arduino Nano)
#define BOOST_PIN 11 // Fixed as PB3 (Pin 11 on Arduino Nano)
#define LED_EXTERNAL 7
#define SERIAL_BAUD 38400
#define EEPROM_TIMER1_PIECEWISE 1 // TODO: Work out bytes and addresses that don't interfere with EEPROM wear leveling 
#define EEPROM_TIMER2_PIECEWISE 82 // 81 bytes for up to 10 points

#define VERSION "0.0.0a"
#define WELCOME_MSG "Bike horn OPTIMISER V" VERSION " started. Compiled " __TIME__ ", " __DATE__

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

void loop() {
    // Wait for the start of the message
    digitalWrite(LED_EXTERNAL, HIGH);
    Serial.find("~`~`");
    while(!Serial.available());
    char instruction = Serial.read();
    digitalWrite(LED_EXTERNAL, LOW);
    uint16_t t1Top, t1Compare;
    uint8_t t2Compare;
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
    }

    // Wait for the end of the message
    Serial.find("`~`~");

    // Acknowledge
    Serial.println(F("~`~`Ack`~`~"));
}

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

void stop() {
    TCCR1A = 0;
    TCCR1B = 0;
    TCCR2A = 0;
    TCCR2B = 0;

    digitalWrite(BOOST_PIN, LOW);
    digitalWrite(PIEZO_PIN, LOW);
}