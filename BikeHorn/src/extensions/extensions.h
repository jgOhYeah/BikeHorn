/** extensions.h
 * Adds an extensions framework for the horn. This file contains the
 * configuration for which extensions are enabled.
 * 
 * Written by Jotham Gates
 * Last modified 01/11/2022
 */
#pragma once
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

#ifdef ACCEL_INSTALLED
#include "burglerAlarm/burglerAlarm.h"
BurglerAlarmExtension burglerAlarm;
#endif

#include "measureBattery.h"
MeasureBatteryExtension measureBattery;

// Array of extensions. This will be the order they appear in the menu if they have menu items.
Extension* extensionsList[] = {
    // &exampleExtension,
    &sosExtension,
#ifdef LOG_RUN_TIME
    &runTimeLogger,
#endif
    &midiSynth,
    &measureBattery,
#ifdef ACCEL_INSTALLED
    &burglerAlarm
#endif
};

// Setting up the extensions manager
Array<Extension*> extensionsArray = {extensionsList, sizeof(extensionsList) / sizeof(Extension*)};
ExtensionManager extensionManager(extensionsArray);