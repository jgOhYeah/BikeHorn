#!/usr/bin/env python3
# GUI libraries
import tkinter as tk                    
from tkinter import ttk
import tkinter.messagebox as tkmb
from typing import Iterable

# Serial
import serial.tools.list_ports
from serial.serialutil import SerialException
import serial

# Sound
import sounddevice as sd

# General
import time
import numpy as np
import threading

class ResultsManager():
    # For generating the functions:
    # 1. Smooth the data
    # 2. Numpy has a piecewise functions tool
    # Have the number of functions as an option in the gui?
    pass

class BikeHornInterface():
    def __init__(self, logging, gui=None):
        self._serial_port = serial.Serial()
        self._logging = logging
        self.set_gui(gui)

    def set_gui(self, gui=None) -> None:
        self._gui = gui
        if self._gui:
            self._gui.add_to_call_on_exit(self.close_port)

    def set_serial_port(self, port_name):
        """Sets and opens the serial port

        Args:
            port_name (str): The serial port to open
        """
        self.close_port()

        self._serial_port.port = port_name
        self._serial_port.baudrate = 38400
        self._serial_port.timeout = 15
        try:
            self._serial_port.open()
        except SerialException:
            msg = "Could not open serial port '{}'".format(port_name)
            self._logging.warning(msg)
            
    def run_test(self, piezo_duty_values:Iterable, boost_duty_values:Iterable, midi_values:Iterable):
        """Runs the test

        Args:
            piezo_duty_values (Iterable): The values to test for the piezo duty cycle (%)
            boost_duty_Values (Iterable): The values to test for the boost duty cycle (%)
            midi_values (Iterable): The values to test for the midi note (0-127)
        """
        self._abort = False

        # Progress bar
        total_steps = len(piezo_duty_values) * len(boost_duty_values) * len(midi_values)
        self._logging.info("Running the test (when it is implemented) with {} points to test".format(total_steps))
        current_step = 0
        
        for midi in midi_values:
            for piezo in piezo_duty_values:
                for boost in boost_duty_values:
                    percent_done = current_step / total_steps * 100
                    # self._logging.info("MIDI Note: {}, Piezo: {}, Boost: {}, {}% done".format(midi, piezo, boost, percent_done))
                    if self._gui:
                        self._gui.update_test_progress(percent_done)
                    
                    time.sleep(0.01)
                    current_step += 1

                    if self._abort:
                        self._logging.info("Aborting the test")
                        self._shutdown(False)
                        return

        self._shutdown()
        self._logging.info("Finished test")

    def get_serial_ports(self):
        """Returns a list of serial ports.
        Based off https://stackoverflow.com/a/67519864
        """
        return [i.device for i in serial.tools.list_ports.comports()]
    
    def close_port(self):
        if self._serial_port.isOpen():
            print("Closing the serial port")
            self._serial_port.close()
    
    def abort_test(self):
        """Sets the flag to stop testing
        """
        self._abort = True
    
    def _shutdown(self, success=True):
        """Attempts to make the horn go quiet and closes the serial port"""
        # TODO
        self._logging.info("Attempted to shut down (when implemented)")

        if self._gui:
            self._gui.test_finished(success)

class GUI():
    def __init__(self, logging, bike_horn):
        """Creates and runs the GUI
        """
        self._logging = logging
        self._bike_horn = bike_horn
        self._call_on_exit = []


    def draw(self):
        """Runs the main loop
        """
        # GUI Setting up
        self._root = tk.Tk()
        self._root.title("Bike Horn Optimiser Tool")
        self._root.tk.call('wm', 'iconphoto', self._root._w, tk.PhotoImage(file='icon.png'))

        # Draw the tabbed layout
        tab_control = ttk.Notebook(self._root)
        run_test_tab = ttk.Frame(tab_control)  
        tab_control.add(run_test_tab, text ='Run test / Serial', )
        save_load_tab = ttk.Frame(tab_control)
        tab_control.add(save_load_tab, text ='Save / Load results')
        view_results_tab = ttk.Frame(tab_control)
        tab_control.add(view_results_tab, text='View results')
        timer1_settings_tab = ttk.Frame(tab_control)
        tab_control.add(timer1_settings_tab, text ='Timer 1 optimisation')
        timer2_settings_tab = ttk.Frame(tab_control)
        tab_control.add(timer2_settings_tab, text ='Timer 2 optimisation')
        upload_settings_tab = ttk.Frame(tab_control)
        tab_control.add(upload_settings_tab, text ='View / Upload Optimisations')
        tab_control.pack(expand=True, fill ='both')

        # Message output
        messages_frame = ttk.Frame(self._root)
        ttk.Label(self._root, text="Messages").pack(side=tk.LEFT, padx=10, pady=10)
        self._text_messages = tk.Text(messages_frame, height=5, padx=5, pady=5)
        self._text_messages.pack(side=tk.LEFT, expand=True, fill='x')
        monitor_scroll_bar = tk.Scrollbar(messages_frame, orient='vertical', command=self._text_messages.yview)
        monitor_scroll_bar.pack(side=tk.RIGHT, fill='y')
        self._text_messages['yscrollcommand'] = monitor_scroll_bar.set
        messages_frame.pack(expand=True, fill='x')
        
        # Fill up the tabs (after drawing the monitor so they can log to it)
        self._draw_run_test_tab(run_test_tab)
        self._draw_save_load_tab(save_load_tab)

        self._root.protocol("WM_DELETE_WINDOW", self._close_window)
    
    def run(self):
        # Ready for interaction
        self._root.mainloop()

    def confirm_run_test(self):
        """Asks whether to run the test as any unsaved results and settings will be lost
        """
        result = tkmb.askyesno("Confirm run test", "Are you sure you want to run a NEW test? Any previous, unsaved results will be lost.")
        if result:
            try:
                # +1 as people would expect top value to be included
                piezo_range = range(int(self._piezo_duty_min_text.get()), int(self._piezo_duty_max_text.get())+1, int(self._piezo_duty_inc_text.get()))
                boost_range = range(int(self._boost_duty_min_text.get()), int(self._boost_duty_max_text.get())+1, int(self._boost_duty_inc_text.get()))
                midi_range = range(int(self._midi_min_text.get()), int(self._midi_max_text.get())+1, int(self._midi_inc_text.get()))
            except ValueError:
                self._logging.error("Error interpreting test parameters as integers")
            else:
                self._bike_horn.set_serial_port(self.serial_port.get())
                test_thread = threading.Thread(target=self._bike_horn.run_test, args=(piezo_range, boost_range, midi_range), daemon=True)
                # TODO: Disable buttons
                # TODO: Cancel button
                test_thread.start()
                self._test_control_button['text'] = "ABORT Test"
                self._test_control_button['command'] = self.confirm_abort_test
    
    def confirm_abort_test(self):
        """Confirms the user is sure they want to abort the test"""
        result = tkmb.askokcancel("Abort the test", "Are you sure you can to abort the test? Any results up until the current note will be saved in {}".format("TODO")) # TODO
        self._bike_horn.abort_test()
    
    def test_finished(self, success):
        self._test_control_button['text'] = "Start Test"
        self._test_control_button['command'] = self.confirm_run_test

        # Set the progress bar to 0 if note successful
        if not success:
            self.update_test_progress(0)

    def update_test_progress(self, value):
        self._root.update_idletasks()
        self._test_progress['value'] = value
    
    def info_dialog(self, msg=""):
        tkmb.showinfo("Info", msg)

    def warning_dialog(self, msg=""):
        tkmb.showwarning("Warning", msg)
    
    def error_dialog(self, msg=""):
        tkmb.showerror("Error", msg)

    def add_monitor_text(self, msg="", end="\n"):
        self._text_messages.configure(state='normal')
        self._text_messages.insert('end', "{}{}".format(msg,end))
        self._text_messages.configure(state='disabled')
        self._text_messages.yview()

    def set_sound_level(self, level):
        try:
            self._root.update_idletasks()
            self._sound_progress['value'] = level
        except RuntimeError:
            print("RuntimeError when setting the sound level, possibly due to an interrupt. Stopping the audio")
            raise sd.CallbackAbort()

    def add_to_call_on_exit(self, method):
        self._call_on_exit.append(method)

    def _draw_run_test_tab(self, tab):
        # Serial port
        ports = self._bike_horn.get_serial_ports()
        self.serial_port = tk.StringVar(self._root)
        ttk.Label(tab, text="Serial port:").grid(row=0, column=0, padx=10, pady=10, sticky=tk.W)
        self._logging.info("Serial ports found: {}".format(ports))
        default = ports[0] if len(ports) else "No serial ports found"
        self._serial_port_dropdown = ttk.OptionMenu(tab, self.serial_port, default, *ports)
        self._serial_port_dropdown.grid(column=1, row=0, padx=10, pady=10, sticky=tk.EW, columnspan=3)

        # Sound device
        # TODO: This feels like it should be put elsewhere
        ttk.Label(tab, text="Audio level (preview):").grid(row=1, column=0, padx=10, pady=10, sticky=tk.W)
        self._sound_progress = ttk.Progressbar(tab, orient=tk.HORIZONTAL, mode='determinate')
        self._sound_progress.grid(column=1, row=1, columnspan=3, sticky=tk.NSEW, padx=10, pady=10)

        # Sweep settings
        # TODO: Increments - duty cycles and midi notes?
        # TODO: Checkbox to include 0?
        ttk.Label(tab, text="Piezo duty min (%):").grid(row=2, padx=10, pady=10, sticky=tk.W)
        self._piezo_duty_min_text = tk.StringVar(value=5)
        piezo_duty_min_spinbox = ttk.Spinbox(tab, from_=0, to=100, textvariable=self._piezo_duty_min_text)
        piezo_duty_min_spinbox.grid(row=2, column=1, padx=10, pady=10, sticky=tk.W)
        ttk.Label(tab, text="Piezo duty max (%):").grid(row=2, column=2, padx=10, pady=10, sticky=tk.W)
        self._piezo_duty_max_text = tk.StringVar(value=75)
        piezo_duty_max_spinbox = ttk.Spinbox(tab, from_=0, to=100, textvariable=self._piezo_duty_max_text)
        piezo_duty_max_spinbox.grid(row=2, column=3, padx=10, pady=10, sticky=tk.W)
        ttk.Label(tab, text="Boost duty min (%):").grid(row=3, padx=10, pady=10, sticky=tk.W)
        self._boost_duty_min_text = tk.StringVar(value=40)
        boost_duty_min_spinbox = ttk.Spinbox(tab,  from_=0, to=100, textvariable=self._boost_duty_min_text)
        boost_duty_min_spinbox.grid(row=3, column=1, padx=10, pady=10, sticky=tk.W)
        ttk.Label(tab, text="Boost duty max (%):").grid(row=3, column=2, padx=10, pady=10, sticky=tk.W)
        self._boost_duty_max_text = tk.StringVar(value=90)
        boost_duty_max_spinbox = ttk.Spinbox(tab,  from_=0, to=100, textvariable=self._boost_duty_max_text)
        boost_duty_max_spinbox.grid(row=3, column=3, padx=10, pady=10, sticky=tk.W)
        ttk.Label(tab, text="MIDI note min:").grid(row=4, padx=10, pady=10, sticky=tk.W)
        self._midi_min_text = tk.StringVar(value=20)
        midi_min_spinbox = ttk.Spinbox(tab,  from_=0, to=127, textvariable=self._midi_min_text)
        midi_min_spinbox.grid(row=4, column=1, padx=10, pady=10, sticky=tk.W)
        ttk.Label(tab, text="MIDI note max:").grid(row=4, column=2, padx=10, pady=10, sticky=tk.W)
        self._midi_max_text = tk.StringVar(value=127)
        midi_max_spinbox = ttk.Spinbox(tab,  from_=0, to=127, textvariable=self._midi_max_text)
        midi_max_spinbox.grid(row=4, column=3, padx=10, pady=10, sticky=tk.W)
        # Increments (6 columns)
        increments_frame = tk.Frame(tab)
        ttk.Label(increments_frame, text="Piezo duty increment (%):").grid(row=0, column=0, padx=10, pady=10, sticky=tk.W)
        self._piezo_duty_inc_text = tk.StringVar(value=5)
        piezo_duty_inc_spinbox = ttk.Spinbox(increments_frame,  from_=0, to=100, textvariable=self._piezo_duty_inc_text, width=10)
        piezo_duty_inc_spinbox.grid(row=0, column=1, padx=10, pady=10, sticky=tk.W)
        ttk.Label(increments_frame, text="Boost duty increment (%):").grid(row=0, column=2, padx=10, pady=10, sticky=tk.W)
        self._boost_duty_inc_text = tk.StringVar(value=5)
        boost_duty_inc_spinbox = ttk.Spinbox(increments_frame,  from_=0, to=100, textvariable=self._boost_duty_inc_text, width=10)
        boost_duty_inc_spinbox.grid(row=0, column=3, padx=10, pady=10, sticky=tk.W)
        ttk.Label(increments_frame, text="MIDI note increment:").grid(row=0, column=4, padx=10, pady=10, sticky=tk.W)
        self._midi_inc_text = tk.StringVar(value=5)
        midi_inc_spinbox = ttk.Spinbox(increments_frame,  from_=0, to=100, textvariable=self._midi_inc_text, width=10)
        midi_inc_spinbox.grid(row=0, column=5, padx=10, pady=10, sticky=tk.W)
        increments_frame.grid(row=5, column=0, columnspan=4, sticky=tk.NSEW)

        # Run test and progress bar
        self._test_control_button = ttk.Button(tab, text="Start test", command=self.confirm_run_test)
        self._test_control_button.grid(row=6, padx=10, pady=10, columnspan=4, sticky=tk.NSEW)

        self._test_progress = ttk.Progressbar(tab, orient=tk.HORIZONTAL, mode='determinate', length=800)
        self._test_progress.grid(column=0, row=7, columnspan=4, sticky=tk.NSEW, padx=10, pady=10)

    def _draw_save_load_tab(self, tab):
        pass

    def _close_window(self):
        """Calls everything this is meant to call before closing, then destroys the window
        """
        self._logging.info("Closing")
        for i in self._call_on_exit:
            i()
        
        # Dodgy bit to make stuff close in the right order
        time.sleep(1)
        self._root.destroy()

class Logging():
    def __init__(self, gui:GUI=None) -> None:
        self._gui = gui

    def set_gui(self, gui=None) -> None:
        self._gui = gui

    def info(self, msg="") -> None:
        print(msg)
        if self._gui:
            self._gui.add_monitor_text(msg)

    def warning(self, msg="") -> None:
        warning_msg = "WARNING: {}".format(msg)
        print(warning_msg)
        if self._gui:
            self._gui.add_monitor_text(warning_msg)
            # self.gui.warning_dialog(msg)
    
    def error(self, msg="") -> None:
        error_msg = "ERROR: {}".format(msg)
        print(error_msg)
        if self._gui:
            self._gui.add_monitor_text(error_msg)
            self._gui.error_dialog(msg)

class AudioLevels():
    def __init__(self, gui:GUI=None):
        self._audio_level = 0
        self._stream = sd.InputStream(callback=self._audio_callback, blocksize=500)
        self.set_gui(gui)
        self._stop_required = False

    def set_gui(self, gui=None) -> None:
        self._gui = gui
        if self._gui:
            self._gui.add_to_call_on_exit(self.stop)

    def start(self):
        self._stream.start()

    def stop(self):
        # Make stopping part of the callback as otherwise it seems to lock up occasionally
        print("Audio stop required")
        self._stop_required = True

    def _audio_callback(self, indata, frames, time, status):
        if self._stop_required:
            print("Stopping audio")
            raise sd.CallbackAbort()
        
        self._audio_level = np.linalg.norm(indata) * 10
        if self._gui:
            self._gui.set_sound_level(self._audio_level)

if __name__ == "__main__":
    logging = Logging()
    bike_horn = BikeHornInterface(logging)
    gui = GUI(logging, bike_horn)
    bike_horn.set_gui(gui)
    audio = AudioLevels(gui)
    logging.set_gui(gui)
    gui.draw()
    audio.start()
    gui.run()