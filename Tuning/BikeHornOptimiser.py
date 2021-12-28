#!/usr/bin/env python3
# GUI libraries
import tkinter as tk                    
from tkinter import ttk
import tkinter.messagebox as tkmb
from serial.serialutil import SerialException

# Serial
import serial.tools.list_ports
import serial

# General
import time

class BikeHornInterface():
    def __init__(self, logging):
        self.serial_port = serial.Serial()
        self.logging = logging

    def set_serial_port(self, port_name):
        """Sets and opens the serial port

        Args:
            port_name (str): The serial port to open
        """
        if self.serial_port.isOpen():
            self.serial_port.close()

        self.serial_port.port = port_name
        self.serial_port.baudrate = 38400
        self.serial_port.timeout = 15
        try:
            self.serial_port.open()
        except SerialException:
            msg = "Could not open serial port '{}'".format(port_name)
            self.logging.warning(msg)
            


    def run_test(self, gui=None):
        """Runs the test
        """
        print("Running the test (when it is implemented)")
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
        self.logging = logging
        self.bike_horn = bike_horn


    def run(self):
        """Runs the main loop
        """
        # GUI Setting up
        self.root = tk.Tk()
        self.root.title("Bike Horn Optimiser Tool")
        self.root.tk.call('wm', 'iconphoto', self.root._w, tk.PhotoImage(file='icon.png'))

        # Draw the tabbed layout
        tab_control = ttk.Notebook(self.root)
        run_test_tab = ttk.Frame(tab_control)  
        tab_control.add(run_test_tab, text ='Run test')
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
        tab_control.pack(expand = True, fill ="both")

        # Monitor output
        monitor_frame = ttk.Frame(self.root)
        ttk.Label(self.root, text="Monitor").pack(side=tk.LEFT, padx=10, pady=10)
        self.text_monitor = tk.Text(monitor_frame, height=5, width=100)
        self.text_monitor.pack(side=tk.LEFT)
        monitor_scroll_bar = tk.Scrollbar(monitor_frame, orient='vertical', command=self.text_monitor.yview)
        monitor_scroll_bar.pack(side=tk.RIGHT, fill='y')
        self.text_monitor['yscrollcommand'] = monitor_scroll_bar.set
        monitor_frame.pack(expand=True, fill='x')
        
        # Fill up the tabs (after drawing the monitor so they can log to it)
        self._draw_run_test_tab(run_test_tab)

        # Ready for interaction
        self.root.mainloop()

    def confirm_run_test(self):
        """Asks whether to run the test as any unsaved results and settings will be lost
        """
        result = tkmb.askyesno("Confirm run test", "Are you sure you want to run a NEW test? Any previous, unsaved results will be lost.")
        if result:
            self.bike_horn.set_serial_port(self.serial_port.get())
            self.bike_horn.run_test(self)
    
    def update_test_progress(self, value):
        self.root.update_idletasks()
        self.test_progress['value'] = value
    
    def info_dialog(self, msg=""):
        tkmb.showinfo("Info", msg)

    def warning_dialog(self, msg=""):
        tkmb.showwarning("Warning", msg)
    
    def error_dialog(self, msg=""):
        tkmb.showerror("Error", msg)

    def add_monitor_text(self, msg="", end="\n"):
        self.text_monitor.configure(state='normal')
        self.text_monitor.insert('end', "{}{}".format(msg,end))
        self.text_monitor.configure(state='disabled')

    def _draw_run_test_tab(self, run_test_tab, ):
        # Serial port
        ports = self.bike_horn.get_serial_ports()
        self.serial_port = tk.StringVar(self.root)
        ttk.Label(run_test_tab, text="Serial port:").grid(row=0, column=0, padx=10, pady=10)
        self.logging.info("Serial ports found: {}".format(ports))
        default = ports[0] if len(ports) else "No serial ports found"
        self.serial_port_dropdown = ttk.OptionMenu(run_test_tab, self.serial_port, default, *ports, )
        self.serial_port_dropdown.grid(column=1, row=0, padx=10, pady=10)
        # Sweep settings
        ttk.Label(run_test_tab, text="TODO: Settings for what to sweep").grid(row=1, padx=10, pady=10)
        ttk.Button(run_test_tab, text="Start test", command=self.confirm_run_test).grid(row=2, padx=10, pady=10)
        self.test_progress_text_top = ttk.Label(run_test_tab, text="MIDI Note Upper")
        self.test_progress_text_top.grid(column=2, row=3, padx=10, pady=10)
        self.test_progress_text_bottom = ttk.Label(run_test_tab, text="MIDI Note Lower")
        self.test_progress_text_bottom.grid(column=0, row=3, padx=10, pady=10)
        self.test_progress_text_current = ttk.Label(run_test_tab, text="MIDI Note Current")
        self.test_progress_text_current.grid(column=1, row=3, padx=10, pady=10)

        self.test_progress = ttk.Progressbar(run_test_tab, orient=tk.HORIZONTAL, mode='determinate')
        self.test_progress.grid_columnconfigure(0, weight=1)
        self.test_progress.grid(column=0, row=4, padx=10, pady=10)

class Logging():
    def __init__(self, gui:GUI=None) -> None:
        self.gui = gui

    def set_gui(self, gui=None) -> None:
        self.gui = gui

    def info(self, msg="") -> None:
        print(msg)
        if self.gui:
            self.gui.add_monitor_text(msg)

    def warning(self, msg="") -> None:
        warning_msg = "WARNING: {}".format(msg)
        print(warning_msg)
        if self.gui:
            self.gui.add_monitor_text(warning_msg)
            # self.gui.warning_dialog(msg)
    
    def error(self, msg="") -> None:
        error_msg = "ERROR: {}".format(msg)
        print(error_msg)
        if self.gui:
            self.gui.add_monitor_text(error_msg)
            self.gui.error_dialog(msg)

if __name__ == "__main__":
    logging = Logging()
    bike_horn = BikeHornInterface(logging)
    gui = GUI(logging, bike_horn)
    logging.set_gui(gui)
    gui.run()