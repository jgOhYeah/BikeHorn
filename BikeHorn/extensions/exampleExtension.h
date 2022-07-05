/** exampleExtension.h
 * An example extension that does nothing except print to the serial console.
 * 
 * Written by Jotham Gates
 * Last modified 04/07/2022
 */

#include "extensionsManager.h"

class ExampleExtension: public Extension {
    public:
        /**
         * @brief Construct a new Example Extension object.
         * 
         * This adds 2 menu items.
         * 
         */
        ExampleExtension() {
            menuActions.length = 2;
            menuActions.array = (MenuItem*)malloc(2*sizeof(MenuItem));
            menuActions.array[0] = (MenuItem)&ExampleExtension::menuAction0;
            menuActions.array[1] = (MenuItem)&ExampleExtension::menuAction1;
        }

        void onStart() {
            printName();
            Serial.println(F("onStart"));
        }

        void onTuneStart() {
            printName();
            Serial.println(F("onTuneStart"));
        }

        void onTuneStop() {
            printName();
            Serial.println(F("onTuneStop"));
        }

        void onWake() {
            printName();
            Serial.println(F("onWake"));
        }

        void onSleep() {
            printName();
            Serial.println(F("onSleep"));
        }
    
    private:
        void printName() {
            Serial.print(F("Example Extension: "));
        }

        void menuAction0() {
            printName();
            Serial.println(F("menuAction0"));
        }

        void menuAction1() {
            printName();
            Serial.println(F("menuAction1"));
        }
};