#!/usr/bin/env python3
import numpy as np
import serial
import sounddevice as sd
import time

def midi_freq(midi):
    # https://newt.phys.unsw.edu.au/jw/notes.html
    return 2**((midi-69)/12) * 440

audio_level = 0
# https://stackoverflow.com/a/48747923
duration = 10 #in seconds

def audio_callback(indata, frames, time, status):
    global audio_level
    audio_level = np.linalg.norm(indata) * 10
    # print(frames)


stream = sd.InputStream(callback=audio_callback, blocksize=500)

ser = serial.Serial('/dev/ttyUSB0', 38400, timeout=15)  # open serial por
boost_duty_range = [0] + list(range(40, 91, 5))
midi_notes = range(33, 71, 1)
piezo_duty_range = range(20, 96, 5)

sound_data = []
adc_data = []

# Record the data
with stream:
    # Notes
    for midi in midi_notes:
        sound_note_data = []
        adc_note_data = []
        piezo_freq = int(midi_freq(midi))

        # Boost duty cycle
        for boost_duty in boost_duty_range:
            boost_duty = (boost_duty * 255) // 100
            sound_boost_data = []
            adc_boost_data = []
            # Piezo duty
            for piezo_duty in piezo_duty_range:
                out = "{},{},{}\n".format(boost_duty, piezo_freq, piezo_duty)
                print(out[:-1], end="")
                ser.write(out.encode('UTF-8'))
                adc = int(ser.readline())
                print(" Sound: {}, ADC: {}".format(audio_level, adc))
                sound_boost_data.append(audio_level)
                adc_boost_data.append(adc)
            sound_note_data.append(sound_boost_data)        
            adc_note_data.append(adc_boost_data)
        sound_data.append(sound_note_data)
        adc_data.append(adc_note_data)

        out = "{},{},{}\n".format(0, 0, 0)
        print("Taking a break between notes to let things cool down: {}".format(out))
        ser.write(out.encode('UTF-8'))
        print(ser.readline())
        # Save each time so as to not loose it
        # piezo_mesh, boost_mesh = np.meshgrid(piezo_duty_range, boost_duty_range)
        sound_data_np = np.array(sound_data)
        adc_data_np = np.array(adc_data)
        np.savez_compressed("Recorded_Results", piezo_duty_range=piezo_duty_range, boost_duty_range=boost_duty_range, sound_data=sound_data_np, adc_data=adc_data_np, midi_notes=midi_notes)
        time.sleep(10)