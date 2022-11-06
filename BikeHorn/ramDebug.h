/** ramDebug.h
 * Functions to print the amount of RAM free (dustance between the head and stack).
 * 
 * Written by Jotham Gates, based off Adafruit examples.
 * Last modified 07/11/2022
 */

#pragma once

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

/**
 * @brief Gets the space between the stack and heap in RAM to help debug stack
 * crashes.
 * 
 * Sourced from
 * https://learn.adafruit.com/memories-of-an-arduino/measuring-free-memory
 * 
 * @return int the space in bytes.
 */
int freeRam() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

/**
 * @brief Prints the line and ram available.
 * 
 * @param lineNumber the line number this function is called from (__LINE__)
 */
void debugRam(unsigned int lineNumber) {
  Serial.print(F("Line "));
  Serial.print(lineNumber);
  Serial.print(F(" RAM Left: "));
  Serial.println(freeRam());
}