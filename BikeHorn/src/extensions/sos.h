/** sos.h
 * Avoid boy who cried wolf situations - FOR EMERGENCIES ONLY!!!
 * 
 * Plays SOS in morse code on repeat (...---...)
 * 
 * Written by Jotham Gates
 * Last modified 01/11/2022
 */
#pragma once
#include "extensionsManager.h"

const uint16_t sosTune[24] PROGMEM = {// Converted from 'sos' by TunePlayer Musescore plugin V1.8.0
    0xe1f4, // Tempo change to 500.00000000000006 BPM
    0x9a3c,0xc038,0x9a3c,0xc038,0x9a3c,0xc038,0x9abc,0xc038,0x9abc,0xc038,
    0x9abc,0xc038,0x9a3c,0xc038,0x9a3c,0xc038,0x9a38,0xc038,0xc078,0xc038,
    0xc078,0xc038,
    0xf001 // End of tune. Restart from the beginning.
};

class SosExtension: public Extension {
    public:
        SosExtension() {
            menuActions.length = 1;
            menuActions.array = (MenuItem*)malloc(sizeof(MenuItem));
            menuActions.array[0] = (MenuItem)&SosExtension::sosMode;
        }
    
    private:
        /**
         * @brief Plays SOS in morse code until mode change is long pressed.
         * 
         */
        void sosMode() {
            // Setup
            Serial.println(F("Playing SOS!!!"));
            startBoost();
            uiBeep(const_cast<uint16_t*>(sosTune));
            
            // Wait for button press
            while (true) {
                WATCHDOG_RESET;
                if (IS_PRESSED(BUTTON_MODE)) {
                    digitalWrite(LED_EXTERNAL, LOW);
                    uint32_t pressTime = modeButtonPress();
                    digitalWrite(LED_EXTERNAL, HIGH);
                    if (pressTime >= LONG_PRESS_TIME) {
                        // Button long pressed. Stop.
                        break;
                    }
                }
                tune.update();
            }

            // Shut down
            Serial.println(F("Stopping SOS!!!"));
            revertToTune();
        }
};