/** extensions.h
 * Adds an extensions framework for the horn. This file contains the
 * configuration for which extensions are enabled.
 * 
 * Written by Jotham Gates
 * Last modified 02/07/2022
 */

#include "extensionsManager.h"

#ifdef LOG_RUN_TIME
#include "logRunTime.h"
RunTimeLogger runTimeLogger;
#endif

Extension* extensionsArray[] = {
#ifdef LOG_RUN_TIME
    &runTimeLogger,
#endif
};
const uint8_t extensionsCount = sizeof(extensionsArray) / sizeof(Extension*);

ExtensionManager extensionManager(extensionsArray, extensionsCount);