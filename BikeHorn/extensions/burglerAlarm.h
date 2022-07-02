/** burglerAlarm.h
 * Requires an accelerometer by installed.
 * When activated, monitors for movement and sets off the siren if too much
 * movement is detected.
 * 
 * Written by Jotham Gates
 * Last modified 01/07/2022
 */

/**
 * @brief Alarm mode
 */
void burglerAlarmMode() {
    // TODO: Implement
    /*
    recharge_time = 60 seconds
    max_count = 3
    Movement count = max_count
    while true:
        Turn accelerometer on
        Take reading
        Use fancy algorithm to compare reading with previous and check if moved significantly.
        If moved significantly:
            decrement movement count
            if count > 0:
                Set off warning beep
            else:
                Go into alarm mode
    */

}

void burglerAlarmSound() {
    // TODO: Implement
    /*
    Sound
    if 2 minutes up or key code entered correctly, go back to burgler alarm mode.*/
}
