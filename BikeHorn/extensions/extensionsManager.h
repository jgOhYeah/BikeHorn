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
        Extension() {}
        
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

    private:
        Extension** extensions;
        const uint8_t count;
};