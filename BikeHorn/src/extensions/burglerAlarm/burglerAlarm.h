/** burglerAlarm.cpp
 * See burglerAlarm.h for more info
 * 
 * Written by Jotham Gates
 * Last modified 01/11/2022
 */
#pragma once
#include "../../../defines.h"
#include <Arduino.h>
#include "../extensionsManager.h"

#define SLEEP_SKIP 255
#define ENCODE_CODE(CODE, LENGTH) (CODE<<4 | LENGTH)

#define MY_CODE ENCODE_CODE(0b1001011, 7)

struct StatesList;

class BurglerAlarmExtension : public Extension {
    public:
        void stateMachine();

        void monitor();

        bool takeReading();

    private:
        /**
         * @brief What state the alarm is in.
         * 
         */
        enum AlarmState {INIT, SLEEP, AWAKE, ALERT, COUNTDOWN_ENTRY, COUNTDOWN, SIREN};

};

/**
 * @brief Class for a state in a state machine
 * 
 */
class State {
    public:
        /**
         * @brief Runs the state.
         * 
         * @return State* the next state to run. To break out of the state
         *         machine, return nullptr.
         */
        virtual State* enter() {}
        const __FlashStringHelper* name;
        static StatesList states;
    protected:
        State(const __FlashStringHelper* name) : name(name) {}
};

class StateInit : public State {
    public:
        StateInit(): State(F("Init")) {}
        virtual State* enter();
};

class StateSleep : public State {
    public:
        StateSleep(): State(F("Sleep")) {}
        virtual State* enter();
};

class StateAwake : public State {
    public:
        StateAwake(): State(F("Awake")) {}
        virtual State* enter();
};

class StateAlert : public State {
    public:
        StateAlert(): State(F("Alert")) {}
        virtual State* enter();
};

class StateCountdown : public State {
    public:
        StateCountdown(): State(F("Countdown")) {}
        virtual State* enter();
};

class StateSiren : public State {
    public:
        StateSiren(): State(F("Sirn")) {}
        virtual State* enter();
};

struct StatesList {
    StateInit* init;
    StateSleep* sleep;
    StateAwake* awake;
    StateAlert* alert;
    StateCountdown* countdown;
    StateSiren* siren;
};

/**
 * @brief Class for entering a pin code.
 * 
 */
class CodeEntry {
    public:
        /**
         * @brief Construct a new Code Entry object.
         * 
         * The code is formatted as a 16 bit binary number. The lowest 4 bits store the number of
         * characters in the code. The remaining upper 12 bits store the characters (0 is the mode
         * button being pressed, 1 is horn button being pressed).
         * 
         * @param code the code to accept as a 16 bit binary number.
         */
        CodeEntry(const uint16_t code);

        /**
         * @brief Construct a new Code Entry object.
         * 
         * The code is formatted as a 16 bit binary number. The lowest 4 bits store the number of
         * characters in the code. The remaining upper 12 bits store the characters (0 is the mode
         * button being pressed, 1 is horn button being pressed).
         * 
         * @param pin the pin to accept (%1010 is horn, mode, horn, mode).
         * @param length the number of digits.
         */
        CodeEntry(const uint16_t pin, const uint8_t length);

        /**
         * @brief Resets code entry so that a new attempt at the code can be entered.
         * 
         */
        void start();

        /**
         * @brief Adds a character to the code.
         * 
         * @param character true for 1, 0 for false
         * @return true if enough characters have been obtained.
         * @return false if not enough characters have been obtained.
         */
        bool add(bool character);
        
        /**
         * @brief Checks the buttons and inputs characters (horn is true, mode is false).
         * 
         * @return true if enough characters have been obtained.
         * @return false is not enough characters have been obtained.
         */
        bool update();

        /**
         * @brief Returns true if the inputted code matches the given code,
         * otherwise false.
         * 
         */
        bool check();

    private:
        const uint16_t m_code;
        int8_t m_charsLeft;
        uint16_t m_inputted;
        uint32_t m_lastPressedTime;
};