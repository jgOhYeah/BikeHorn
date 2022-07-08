/** logRunTime.h
 * Logs the number of times the horn has been used and how long for to EEPROM.
 * Designed to help when determining how long the batteries last.
 * 
 * This was previously part of the main program, but has been turned into an
 * extension for maintainability.
 * 
 * Written by Jotham Gates
 * Last modified 08/07/2022
 */

#include "extensionsManager.h"
#include <EEPROMWearLevel.h>

#define LOG_VERSION 3
#define EEPROM_WEAR_LEVEL_LENGTH 1024 - 2*EEPROM_PIECEWISE_SIZE // Leave enough space at the end for the optimiser settings

class RunTimeLogger: public Extension {
    public:
        RunTimeLogger() {
            menuActions.length = 1;
            menuActions.array = (MenuItem*)malloc(sizeof(MenuItem));
            menuActions.array[0] = (MenuItem)&RunTimeLogger::resetEEPROM;
        }

        void onStart() {
            EEPROMwl.begin(LOG_VERSION, 2, EEPROM_WEAR_LEVEL_LENGTH);
            Serial.print(F("Run time logging enabled. Horn has been sounding for "));
            Serial.print(getTime() / 1000);
            Serial.println(F(" seconds."));
            Serial.print(F("The horn has been used "));
            Serial.print(getBeeps());
            Serial.println(F(" times."));
        }

        void onTuneStart() {
            wakeTime = millis();
        }

        void onTuneStop() {
            addTime(millis() - wakeTime);
            addBeep();
        }
    
    private:
        uint32_t wakeTime;

        /** @returns the time the horn has been sounding in ms */
        inline uint32_t getTime() {
            uint32_t time;
            EEPROMwl.get(0, time);
            return time;
        }

        /** Add @param time in ms to the total time the horn has been sounding */
        inline void addTime(uint32_t time) {
            time += getTime();
            EEPROMwl.put(0, time);
        }

        /** @returns the number of times the horn has gone off */
        inline uint16_t getBeeps() {
            uint16_t beeps;
            EEPROMwl.get(1, beeps);
            return beeps;
        }

        /** Adds 1 to the number of times the horn has gone off */
        inline void addBeep() {
            uint16_t beeps = getBeeps() + 1;
            EEPROMwl.put(1, beeps);
        }

        /** Resets stored data to 0 */
        inline void resetEEPROM() {
            Serial.println(F("Wiping run times"));
            uiBeep(const_cast<uint16_t*>(beeps::error));
            EEPROMwl.put(0, (uint32_t)0);
            EEPROMwl.put(1, (uint16_t)0);
        }
};