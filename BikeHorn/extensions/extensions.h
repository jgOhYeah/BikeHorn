/** extensions.h
 * Adds an extensions framework for the horn. This file contains the
 * configuration for which extensions are enabled.
 * 
 * Written by Jotham Gates
 * Last modified 02/07/2022
 */

#include "extensionsManager.h"

// Extension files and objects
#ifdef LOG_RUN_TIME
#include "logRunTime.h"
RunTimeLogger runTimeLogger;
#endif

// #include "exampleExtension.h"
// ExampleExtension exampleExtension;

#include "sos.h"
SosExtension sosExtension;

#include "midiSynth.h"
MidiSynthExtension midiSynth;

// Array of extensions. This will be the order they appear in the menu if they have menu items.
Extension* extensionsArray[] = {
    // &exampleExtension,
    &sosExtension,
#ifdef LOG_RUN_TIME
    &runTimeLogger,
#endif
    &midiSynth
};

// Setting up the extensions manager
const uint8_t extensionsCount = sizeof(extensionsArray) / sizeof(Extension*);
ExtensionManager extensionManager(extensionsArray, extensionsCount);