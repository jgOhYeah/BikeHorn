/** midiSynth.h
 * Allows the horn to be used as a midi synthesiser. Hold the change mode
 * button as the horn is reset to enable it.
 * 
 * This was previously part of the main program, but has been turned into an
 * extension for maintainability.
 * 
 * Written by Jotham Gates
 * Last modified 08/07/2022
 */

#include "extensionsManager.h"

class MidiSynthExtension: public Extension {
    public:
        void onStart() {
            // Go to midi synth mode if change mode pressed or reset the eeprom.
            if(!digitalRead(BUTTON_MODE)) {
                midiSynth();
            }
        }


    private:
        /** Mode for a midi synth. This is blocking and will only exit on reset or if the horn button is pressed. */
        void midiSynth() {
            // Flashing lights to warn of being in this mode
            digitalWrite(LED_BUILTIN, LOW);
            digitalWrite(LED_EXTERNAL, HIGH);

            startBoost();
            uint8_t currentNote;

            while(digitalRead(BUTTON_HORN)) {
                WATCHDOG_RESET;
                uint8_t incomingNote; // The next byte to be read off the serial buffer.
                uint8_t midiPitch; // The MIDI note number.
                uint8_t midiVelocity; //The velocity of the note.

                incomingNote = getByte();
                // Analyse the byte. The byte at this stage should be a status byte. If it isn't, the byte is ignored.
                switch (incomingNote) {
                    case B10010000 | MIDI_CHANNEL: // Note on
                    midiPitch = getByte();
                    if(midiPitch != -1) {
                        midiVelocity = getByte();
                        if(midiVelocity != -1) {
                            piezo.playMidiNote(midiPitch); // This will handle the conversion into note and octave.
                            digitalWrite(LED_BUILTIN, HIGH);
                            digitalWrite(LED_EXTERNAL, LOW);
                            currentNote = midiPitch;
                        }
                    }
                    break;

                    case B10000000 | MIDI_CHANNEL: // Note off
                    midiPitch = getByte();
                    if(midiPitch == currentNote) {
                        piezo.stopSound();
                        digitalWrite(LED_BUILTIN, LOW);
                        digitalWrite(LED_EXTERNAL, HIGH);
                    }
                    break;
                }
            }
            piezo.stopSound();
            digitalWrite(LED_BUILTIN, LOW);
            digitalWrite(LED_EXTERNAL, LOW);
        }

        /** Wait for an incoming note and return the note. */
        byte getByte() {
            while (!Serial.available() && digitalRead(BUTTON_HORN)) {
                WATCHDOG_RESET;
            } //Wait while there are no bytes to read in the buffer.
            return Serial.read();
        }
};