/** extensionsManager.h
 * Adds an extensions framework for the horn. This file contains the base
 * classes for extensions.
 * 
 * Written by Jotham Gates
 * Last modified 02/07/2022
 */
#pragma once

typedef void (*MenuItem)();

template <typename Type>
struct Array {
    Type* array;
    uint8_t length;
};

class Extension {
    public:
        /**
         * @brief Construct a new Extension object.
         */
        Extension() {
            menuActions.length = 0;
        }
        
        /**
         * @brief Called when the horn starts up.
         * 
         */
        virtual void onStart() {}

        /**
         * @brief Called when the horn wakes up (to change tune of play a tune).
         * 
         */
        virtual void onWake() {}

        /**
         * @brief Called before the hown goes to sleep.
         * 
         */
        virtual void onSleep() {}

        /**
         * @brief Called before the horn starts making noise.
         * 
         */
        virtual void onTuneStart() {}

        /**
         * @brief Called after the horn stops playing (horn button released).
         * 
         */
        virtual void onTuneStop() {}

        Array<MenuItem> menuActions;
};

/**
 * @brief Class for handling extensions.
 * 
 */
class ExtensionManager {
    public:
        ExtensionManager(Extension* extensionsArray[], const uint8_t extensionsCount) : extensions(extensionsArray), count{extensionsCount} {}

        /**
         * @brief Calls all extensions on startup.
         * 
         */
        void callOnStart() {
            Serial.print(F("There are "));
            Serial.print(count);
            Serial.println(F(" extensions installed"));
            for (uint8_t i = 0; i < count; i++) {
                extensions[i]->onStart();
            }
        }

        /**
         * @brief Calls all extensions upon waking up.
         * 
         */
        void callOnWake() {
            for (uint8_t i = 0; i < count; i++) {
                extensions[i]->onWake();
            }
        }

        /**
         * @brief Calls all extensions upon going to sleep.
         * 
         */
        void callOnSleep() {
            for (uint8_t i = 0; i < count; i++) {
                extensions[i]->onSleep();
            }
        }

        /**
         * @brief Calls all extensions just before playing a tune.
         * 
         */
        void callOnTuneStart() {
            for (uint8_t i = 0; i < count; i++) {
                extensions[i]->onTuneStart();
            }
        }

        /**
         * @brief Calls all extensions just before playing a tune.
         * 
         */
        void callOnTuneStop() {
            for (uint8_t i = 0; i < count; i++) {
                extensions[i]->onTuneStop();
            }
        }

        /**
         * @brief Shows the menu.
         * 
         */
        void displayMenu() {
            Serial.print(F("Displaying menu with "));
            uint8_t items = countMenuItems();
            Serial.print(items);
            Serial.println(F(" items."));
            if (items != 0) {
                // There is at least one item in the menu
                uint32_t lastInteractionTime = millis();
                uint8_t selected = 0;
                while (millis() - lastInteractionTime < MENU_TIMEOUT) {
                    WATCHDOG_RESET;
                    if (!digitalRead(BUTTON_MODE)) {
                        digitalWrite(LED_EXTERNAL, LOW);
                        uint32_t pressTime = modeButtonPress();
                        digitalWrite(LED_EXTERNAL, HIGH);
                        if (!pressTime) {
                            // Horn button pressed. Exit.
                            return;
                        } else if (pressTime < LONG_PRESS_TIME) {
                            // Button short pressed. Increment the option.
                            lastInteractionTime = millis();
                            selected++;
                            if (selected == items) {
                                selected = 0;
                            }
                        } else {
                            // Button long pressed. Run the given function.
                            runMenuItem(selected);
                            return;
                        }
                    }
                    if (!digitalRead(BUTTON_HORN)) {
                        return;
                    }
                }
            }
            // Timed out or no extensions with menu items enabled.
            Serial.println(F("Timed out."));
        }

    private:
        Extension** extensions;
        const uint8_t count;

        /**
         * @brief Calculates and returns the number of items in the menu.
         * 
         * @return uint8_t The number of menu items.
         */
        uint8_t countMenuItems() {
            uint8_t items = 0;
            for (uint8_t i = 0; i < count; i++) {
                items += extensions[i]->menuActions.length;
            }
            return items;
        }

        /**
         * @brief Finds and runs the given menu item.
         * 
         * @param index The index in the menu.
         */
        void runMenuItem(int8_t index) {
            // Convert the menu index into an extension and index in the extension.
            uint8_t current = 0;
            uint8_t extension = 0;
            uint8_t next = current + extensions[extension]->menuActions.length;
            while (index >= next) {
                current = next;
                extension++;
                next = current + extensions[extension]->menuActions.length;
            }
            uint8_t extensionIndex = index - current;

            // Do the running
            Serial.print(F("Running menu item "));
            Serial.print(extensionIndex);
            Serial.print(F(" of extension "));
            Serial.print(extension);
            Serial.print(F(" that appeared in the menu as item "));
            Serial.println(index);
            extensions[extension]->menuActions.array[extensionIndex]();
        }
};