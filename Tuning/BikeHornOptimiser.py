#!/usr/bin/env python3
# GUI libraries
import tkinter as tk                    
from tkinter import ttk
import tkinter.messagebox as tkmb

# Serial
import serial.tools.list_ports
from serial.serialutil import SerialException
import serial

# Sound
import sounddevice as sd

# General
import time
import numpy as np

class BikeHornInterface():
    def __init__(self, logging):
        self._serial_port = serial.Serial()
        self._logging = logging

    def set_serial_port(self, port_name):
        """Sets and opens the serial port

        Args:
            port_name (str): The serial port to open
        """
        if self._serial_port.isOpen():
            self._serial_port.close()

        self._serial_port.port = port_name
        self._serial_port.baudrate = 38400
        self._serial_port.timeout = 15
        try:
            self._serial_port.open()
        except SerialException:
            msg = "Could not open serial port '{}'".format(port_name)
            self._logging.warning(msg)
            


    def run_test(self, gui=None):
        """Runs the test
        """
        self._logging.info("Running the test (when it is implemented)")
        for i in range(0, 101, 1):
            time.sleep(0.1)
            if gui:
                gui.update_test_progress(i)
        
        # TODO
    
    def get_serial_ports(self):
        """Returns a list of serial ports.
        Based off https://stackoverflow.com/a/67519864
        """
        return [i.device for i in serial.tools.list_ports.comports()]
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
        tab_control.add(run_test_tab, text ='Run test', )
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
            self._bike_horn.set_serial_port(self.serial_port.get())
            self._bike_horn.run_test(self)
    
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
        # TODO: Put it here?
        ttk.Label(tab, text="Audio level:").grid(row=1, column=0, padx=10, pady=10, sticky=tk.W)
        self._sound_progress = ttk.Progressbar(tab, orient=tk.HORIZONTAL, mode='determinate')
        self._sound_progress.grid(column=1, row=1, columnspan=3, sticky=tk.NSEW, padx=10, pady=10)

        # Sweep settings
        ttk.Label(tab, text="Piezo duty min (%):").grid(row=2, padx=10, pady=10, sticky=tk.W)
        ttk.Spinbox(tab).grid(row=2, column=1, padx=10, pady=10, sticky=tk.W)
        ttk.Label(tab, text="Piezo duty max (%):").grid(row=2, column=2, padx=10, pady=10, sticky=tk.W)
        ttk.Spinbox(tab).grid(row=2, column=3, padx=10, pady=10, sticky=tk.W)
        ttk.Label(tab, text="Boost duty min (%):").grid(row=3, padx=10, pady=10, sticky=tk.W)
        ttk.Spinbox(tab).grid(row=3, column=1, padx=10, pady=10, sticky=tk.W)
        ttk.Label(tab, text="Boost duty max (%):").grid(row=3, column=2, padx=10, pady=10, sticky=tk.W)
        ttk.Spinbox(tab).grid(row=3, column=3, padx=10, pady=10, sticky=tk.W)
        ttk.Label(tab, text="MIDI note min:").grid(row=4, padx=10, pady=10, sticky=tk.W)
        ttk.Spinbox(tab).grid(row=4, column=1, padx=10, pady=10, sticky=tk.W)
        ttk.Label(tab, text="MIDI note max:").grid(row=4, column=2, padx=10, pady=10, sticky=tk.W)
        ttk.Spinbox(tab).grid(row=4, column=3, padx=10, pady=10, sticky=tk.W)

        # Run test and progress bar
        ttk.Button(tab, text="Start test", command=self.confirm_run_test).grid(row=5, padx=10, pady=10, columnspan=4, sticky=tk.NSEW)

        self._test_progress = ttk.Progressbar(tab, orient=tk.HORIZONTAL, mode='determinate', length=800)
        self._test_progress.grid(column=0, row=6, columnspan=4, sticky=tk.NSEW, padx=10, pady=10)

    def _draw_save_load_tab(self, tab):
        pass

    def _close_window(self):
        """Calls everything this is meant to call before closing, then destroys the window
        """
        self._logging.info("Closing")
        for i in self._call_on_exit:
            i()
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

    def set_gui(self, gui=None) -> None:
        self._gui = gui
        if self._gui:
            self._gui.add_to_call_on_exit(self.stop)

    def start(self):
        self._stream.start()

    def stop(self):
        print("Stopping audio")
        self._stream.abort()
        print("Done")

    def _audio_callback(self, indata, frames, time, status):
        self._audio_level = np.linalg.norm(indata) * 10
        if self._gui:
            self._gui.set_sound_level(self._audio_level)

if __name__ == "__main__":
    logging = Logging()
    bike_horn = BikeHornInterface(logging)
    gui = GUI(logging, bike_horn)
    audio = AudioLevels(gui)
    logging.set_gui(gui)
    gui.draw()
    audio.start()
    gui.run()