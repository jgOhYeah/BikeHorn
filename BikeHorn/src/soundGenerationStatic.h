/** soundGenerationStatic.h
 * Static methods and ISRs for soundGeneration.h.
 * 
 * This allows soundGeneration.h to be included multiple times as needed without
 * too many definitions of these methods.
 * 
 * Written by Jotham Gates
 * 
 * Last modified 23/02/2024
 */
#pragma once
#include "soundGeneration.h"

volatile uint16_t BikeHornSound::nextTop;
volatile uint16_t BikeHornSound::nextComp;

/**
 * Interrupt for timer overflow to change the frequency of the warble safely at the correct time in the cycle without
 * using a double buffered register. The manual suggests OCR1A should be set as top as it is double buffered, however
 * this will disable PWM on the pin I am using.
 * 
 * The risk of changing ICR1 randomly at any time is that a reset of the counter only happens when the counter equals
 * ICR1. This means there is a chance that it could be changed to a value that the counter is currently above. The
 * counter would then have to count to 0xFFFF and overflow to get back to normal operation, causing a noticable glitch
 * that makes you question the reliability of the horn.
 * 
 * This ISR will only change ICR1 as it is resetting, hopefully avoiding this issue (did someone mention software
 * solution to hardware problem?)
 */
ISR(TIMER1_OVF_vect) {
    ICR1 = BikeHornSound::nextTop;
    OCR1A = BikeHornSound::nextComp;
    TIMSK1 = 0; // Disable timer 1 interrupts
}