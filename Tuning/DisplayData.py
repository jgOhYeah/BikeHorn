#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider
import sys

def midi_freq(midi):
    """ Converts a midi note to a frequency. See https://newt.phys.unsw.edu.au/jw/notes.html """
    return 2**((midi-69)/12) * 440

def midi_top(midi):
    """ Returns what timer 1 should count up to for a given midi note """
    clock_freq = 16000000
    prescalar = 8
    return int(clock_freq / prescalar / midi_freq(midi)) - 1

def duty_compare(duty, top):
    """ Returns the compare value for a given duty cycle and top value.
    Duty is in % """
    return int(duty * top / 100)

def apply_to_all(function, *args):
    """ Element wise operations """
    length = len(args[0])
    output = np.empty(length)
    for i in range(length):
        single_args = []
        for j in range(len(args)):
            single_args.append(args[j][i])

        output[i] = function(*single_args)
    
    return output

def print_array(arr):
    for i in arr:
        print("{:>6.0f}".format(i), end="")

def linear(x1, y1, x2, y2):
    """ Returns m and c from y = mx + c """
    # y-y1 = m(x-x1)
    # m = (y2 - y1) / (x2 - x1)
    # y = (y2 - y1) / (x2 - x1) * (x - x1) + y1
    m = (y2 - y1) / (x2 - x1)
    c = y1 - m*x1
    return m, c

def mapping(x, y, dtype, invar, outvar):
    """ Works out a linear function for each range """
    result = "{} {};\n".format(dtype, outvar)
    result += "if({} > {:.0f}) {{\n    {} = {:.0f};\n}} ".format(invar, x[0], outvar, y[0])
    for i in range(1, len(x)):
        m, c = linear(x[i-1], y[i-1], x[i], y[i])
        if m == 0:
            result += "else if({} > {:.0f}) {{\n    {} = {:.0f};\n}} ".format(invar, x[i], outvar, c)
        elif m >= 1:
            result += "else if({} > {:.0f}) {{\n    {} = {:.0f}*{} + {:.0f};\n}} ".format(invar, x[i], outvar, m, invar, c)
        elif m <= -1:
            result += "else if({} > {:.0f}) {{\n    {} = {:.0f} - {:.0f}*{};\n}} ".format(invar, x[i], outvar, c, -m, invar)
        elif m > 0:
            result += "else if({} > {:.0f}) {{\n    {} = {}/{:.0f} + {:.0f};\n}} ".format(invar, x[i], outvar, invar, 1/m, c)
        else: # -1 < m < 0
            result += "else if({} > {:.0f}) {{\n    {} = {:.0f} - {}/{:.0f};\n}} ".format(invar, x[i], outvar, c, invar, -1/m)
    result += "else {{\n    {} = {:.0f};\n}}".format(outvar, y[-1])
    return result

# Load data
filename = "Recorded_Results.npz"
if len(sys.argv) > 1:
    filename = sys.argv[1]

data = np.load(filename)
piezo_mesh, boost_mesh = np.meshgrid(data['piezo_duty_range'], data['boost_duty_range'])
sound_data = data['sound_data']
adc_data = data['adc_data']
midi_notes = data['midi_notes']

# Based of various examples
# Set up a figure twice as wide as it is tall
fig_3d = plt.figure(figsize=plt.figaspect(0.5))
# Set up the axes for the first plot
ax_loudness = fig_3d.add_subplot(1, 2, 1, projection='3d')
# Second ADC Plot
ax_adc = fig_3d.add_subplot(1, 2, 2, projection='3d')

# Slider for midi note (https://matplotlib.org/stable/gallery/widgets/slider_demo.html)
# Adjust the main plot to make room for the sliders
plt.subplots_adjust(bottom=0.25)
# Make a horizontal slider to control the frequency.
axfreq = plt.axes([0.25, 0.1, 0.65, 0.03])
freq_slider = Slider(
    ax=axfreq,
    label='Record Index',
    valmin=0,
    valmax=len(sound_data)-1,
    valinit=0,
  # Set up the axes for the first plot
    valstep=1
)

def update_plot(val):
    # Update the title
    fig_3d.suptitle("Performance for midi note {}".format(midi_notes[val]))

    # Process data and get the maximum and minimum
    loudness = sound_data[val]
    adc = adc_data[val]

    # Display subplot 1
    ax_loudness.cla()
    ax_loudness.set_xlabel("Piezo Duty Cycle [%]")
    ax_loudness.set_ylabel("Boost Duty Cycle [%]")
    ax_loudness.set_zlabel("Loudness")
    ax_loudness.set_title("Microphone level")
    loud_surf = ax_loudness.plot_surface(piezo_mesh, boost_mesh, loudness, color="b")
    ax_loudness.set_xlim3d(piezo_mesh[0,0], piezo_mesh[-1,-1])
    ax_loudness.set_ylim3d(boost_mesh[0,0], boost_mesh[-1,-1])

    # Display subplot 2
    ax_adc.cla()
    ax_adc.set_xlabel("Piezo Duty Cycle [%]")
    ax_adc.set_ylabel("Boost Duty Cycle [%]")
    ax_adc.set_zlabel("ADC [0:1023]")
    ax_adc.set_title("Boost stage ADC")
    adc_surf = ax_adc.plot_surface(piezo_mesh, boost_mesh, adc, color='r')
    ax_adc.set_xlim3d(piezo_mesh[0,0], piezo_mesh[-1,-1])
    ax_adc.set_ylim3d(boost_mesh[0,0], boost_mesh[-1,-1])

    # Update
    fig_3d.canvas.draw_idle()

update_plot(0)
freq_slider.on_changed(update_plot)

# Make the axis move together (https://stackoverflow.com/a/41616868)
def on_move(event):
    if event.inaxes == ax_loudness:
        if ax_loudness.button_pressed in ax_loudness._rotate_btn:
            ax_adc.view_init(elev=ax_loudness.elev, azim=ax_loudness.azim)
        elif ax_loudness.button_pressed in ax_loudness._zoom_btn:
            ax_adc.set_xlim3d(ax_loudness.get_xlim3d())
            ax_adc.set_ylim3d(ax_loudness.get_ylim3d())
            ax_adc.set_zlim3d(ax_loudness.get_zlim3d())
    elif event.inaxes == ax_adc:
        if ax_adc.button_pressed in ax_adc._rotate_btn:
            ax_loudness.view_init(elev=ax_adc.elev, azim=ax_adc.azim)
        elif ax_adc.button_pressed in ax_adc._zoom_btn:
            ax_loudness.set_xlim3d(ax_adc.get_xlim3d())
            ax_loudness.set_ylim3d(ax_adc.get_ylim3d())
            ax_loudness.set_zlim3d(ax_adc.get_zlim3d())
    else:
        return
    fig_3d.canvas.draw_idle()

c1 = fig_3d.canvas.mpl_connect('motion_notify_event', on_move)

plt.show(block=False)

# Calculate and display the best coordinates
best_boost_duty = [0] * len(sound_data)
best_piezo_duty = [0] * len(sound_data)
best_loudness = np.empty(len(sound_data))
corresponding_adc = np.empty(len(sound_data))

for i in range(len(sound_data)):
    boost_index, piezo_index = np.unravel_index(np.argmax(sound_data[i], axis=None), sound_data[i].shape)
    best_boost_duty[i] = boost_mesh[boost_index, 0]
    best_piezo_duty[i] = piezo_mesh[0, piezo_index]
    best_loudness[i] = sound_data[i, boost_index, piezo_index]
    corresponding_adc[i] = adc_data[i, boost_index, piezo_index]

    # print("Midi: {}\tLoudness: {}\tBest boost: {}\tBest piezo: {}".format(midi_notes[i], sound_data[i, boost_index, piezo_index], best_boost_duty[i], best_piezo_duty[i]))

boost_x = np.array([21, 47, 57, 95, 110, 127])
boost_y = np.array([72, 72, 85, 85, 60, 60])
piezo_x = np.array([21, 50, 90, 127])
piezo_y = np.array([95, 95, 8, 8])

# Display
fig_best_params = plt.figure()
fig_best_params.suptitle("Parameters for best performance of each note")
# Boost duty cycle
ax_boost_best = fig_best_params.add_subplot(2, 1, 1)
ax_boost_best.plot(midi_notes[:len(sound_data)], best_boost_duty, label="Measured")
ax_boost_best.plot(boost_x, boost_y, color="orange", label="Map")
ax_boost_best.set_title("Optimal boost duty cycle")
ax_boost_best.set_xlabel("MIDI Note")
ax_boost_best.set_ylabel("Boost duty cycle [%]")
ax_boost_best.legend()

ax_piezo_best = fig_best_params.add_subplot(2, 1, 2)
ax_piezo_best.plot(midi_notes[:len(sound_data)], best_piezo_duty, label="Measured")
ax_piezo_best.plot(piezo_x, piezo_y, color="orange", label="Map")
ax_piezo_best.set_title("Optimal piezo duty cycle")
ax_piezo_best.set_xlabel("MIDI Note")
ax_piezo_best.set_ylabel("Piezo duty cycle [%]")
ax_piezo_best.legend()

plt.subplots_adjust(hspace=0.5)
plt.show(block=False)

# Print with the x axis being timer 1 max instead of midi
# Transform all coordinates to the new axes
boost_x_count = apply_to_all(midi_top, boost_x)
boost_y_count = apply_to_all(duty_compare, boost_y, [0xFF]*len(boost_y))
piezo_x_count = apply_to_all(midi_top, piezo_x)
piezo_y_count = apply_to_all(duty_compare, piezo_y, piezo_x_count)
t1_counts = apply_to_all(midi_top, midi_notes[:len(sound_data)])
t1_bbd = apply_to_all(duty_compare, best_boost_duty, [0xFF]*len(best_boost_duty)) # Top of timer 2 doesn't change
t1_bpd = apply_to_all(duty_compare, best_piezo_duty, t1_counts)

print("Mapping (linear interpolation):")
print("Timer 2:")
print("    T1 Top:     ", end="")
print_array(boost_x_count)
print("\n    T2 Compare: ", end="")
print_array(boost_y_count)
print("\nTimer 1:")
print("    T1 Top:     ", end="")
print_array(piezo_x_count)
print("\n    T1 Compare: ", end="")
print_array(piezo_y_count)
print("\n")

# TODO:
# Work out a linear function for each section and attempt to generate c++ code for it.
print("\n// For Timer 2: (May need to be adjusted / optimised by hand")
print(mapping(boost_x_count, boost_y_count, "uint16_t", "counter", "timer2Comp"))

print("\n// For Timer 1: (May need to be adjusted / optimised by hand")
print(mapping(piezo_x_count, piezo_y_count, "uint16_t", "counter", "timer1Comp"))

# Display
fig_bp_count = plt.figure()
fig_bp_count.suptitle("Parameters for best performance of each note")
# Boost duty cycle
ax_bb_count = fig_bp_count.add_subplot(2, 1, 1)
ax_bb_count.plot(t1_counts, t1_bbd, label="Measured")
ax_bb_count.plot(boost_x_count, boost_y_count, color="orange", label="Map")
ax_bb_count.set_title("Optimal timer 2 compare value")
ax_bb_count.set_xlabel("Timer 1 max")
ax_bb_count.set_ylabel("Timer 2 compare value")
ax_bb_count.legend()

ax_pb_count = fig_bp_count.add_subplot(2, 1, 2)
ax_pb_count.plot(t1_counts, t1_bpd, label="Measured")
ax_pb_count.plot(piezo_x_count, piezo_y_count, color="orange", label="Map")
ax_pb_count.set_title("Optimal timer 1 compare value")
ax_pb_count.set_xlabel("Timer 1 max")
ax_pb_count.set_ylabel("Timer 1 compare value")
ax_pb_count.legend()

plt.subplots_adjust(hspace=0.5)
plt.show(block=False)

# Print with the x axis being the frequency instead of midi #
freqs = midi_freq(midi_notes[:len(sound_data)])
# Display
fig_best_params_f = plt.figure()
fig_best_params_f.suptitle("Parameters for best performance of each note")

# Boost duty cycle
ax_boost_best_f = fig_best_params_f.add_subplot(2, 2, 1)
ax_boost_best_f.plot(freqs, best_boost_duty, color='r')
ax_boost_best_f.set_title("Optimal boost duty cycle")
ax_boost_best_f.set_xlabel("Frequency [Hz]")
ax_boost_best_f.set_ylabel("Boost duty cycle [%]")

# Piezo Duty cycle
ax_piezo_best_f = fig_best_params_f.add_subplot(2, 2, 3)
ax_piezo_best_f.plot(freqs, best_piezo_duty, color='r')
ax_piezo_best_f.set_title("Optimal boost duty cycle")
ax_piezo_best_f.set_xlabel("Frequency [Hz]")
ax_piezo_best_f.set_ylabel("Piezo duty cycle [%]")

# Loudness
ax_loudness_best_f = fig_best_params_f.add_subplot(2, 2, 2)
ax_loudness_best_f.plot(freqs, best_loudness, color='g')
ax_loudness_best_f.set_title("Microphone level at optimal")
ax_loudness_best_f.set_xlabel("Frequency [Hz]")
ax_loudness_best_f.set_ylabel("Boost duty cycle [%]")

# ADC
ax_adc_f = fig_best_params_f.add_subplot(2, 2, 4)
ax_adc_f.plot(freqs, corresponding_adc, color='g')
ax_adc_f.set_title("Boost ADC at optimal")
ax_adc_f.set_xlabel("Frequency [Hz]")
ax_adc_f.set_ylabel("Piezo duty cycle [%]")

plt.subplots_adjust(hspace=0.5, wspace=0.4, top=0.88, bottom=0.11, left=0.11, right=0.96)
plt.show()
