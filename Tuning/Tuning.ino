#define LED_EXTERNAL 7
#define BOOST_ADC A0

void setup() {
    Serial.begin(38400);
    //Serial.setTimeout(65535);
    // Serial.println(F("BoostDuty,PiezoFrequency,PiezoCountMax,PiezoDuty,PiezoDutyCount,BoostADC"));
    // ADC setup
    analogReference(INTERNAL);

    // Indicator LEDs setup
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_EXTERNAL, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_EXTERNAL, HIGH);

    // Boost PWM Setup
    DDRB |= (1 << PB3); // Set PB3 to be an output (Pin11 Arduino UNO)
    TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20); // Mode 1, fast PWM, reset at 0xff
    TCCR2B = (1<< CS20); // Prescalar 1
    OCR2A = 0;

    // Piezo PWM Setup
    DDRB |= (1 << PB1); // Set PB1 to be an output (Pin9 Arduino UNO)
    // Setup non inverting mode (duty cycle is sensible), fast pwm mode 14
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS11); // With prescalar 8 (with a clock frequency of 16MHz, can get all notes required)
    ICR1 = 0; // Maximum to count to
    OCR1A = 0; // Duty cycle
}

void loop() {
    // Shut down while waiting for input
    //OCR2A = 0; // 0 Duty cycle
    //ICR1 = 0;

    // Get inputs
    digitalWrite(LED_EXTERNAL, HIGH);
    uint8_t boostDuty = Serial.parseInt();
    uint16_t piezoFreq = Serial.parseInt();
    uint16_t piezoCountMax = 2000000L / piezoFreq;
    uint32_t dutyCompare = (uint32_t)Serial.parseInt() * (uint32_t)piezoFreq / 100;

    // Set the registers to actually generate the pwm required.
    OCR1A = dutyCompare;
    OCR2A = boostDuty;
    ICR1 = piezoCountMax;

    // Say bust and wait to stabilise
    digitalWrite(LED_EXTERNAL, LOW);
    delay(200);

    // Report back and clear and junk data
    Serial.println(analogRead(BOOST_ADC));
    while(Serial.available()) Serial.read();
}