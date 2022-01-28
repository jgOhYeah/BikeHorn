I find piezo sirens to be fairly fussy and unpredictable when trying to make them loud, especially across a broad range of frequencies. To help with this, I have written a small application to vary a couple of parameters under the horn's control and measure the resulting volume of the siren using a laptop / computer microphone. The data is then analysed and converted into a series of straight lines before being uploaded to the EEPROM of the Arduino in the horn.

This application is a python script and has been tested on Windows and Linux (Ubuntu), but should also work on Macintosh / other operating systems as long as the required libraries are installed.

> **Note:** This optimiser, the files it produces / uses and the way data is stored on the horn itself is in no way compatible with the earlier `SoundLevel.py` and `DisplayData.py` scripts. If you want to use or access these tools, you will need to use an [earlier release](https://github.com/jgOhYeah/BikeHorn/releases/tag/v1.0.0).

# Table of contents <!-- omit in toc -->
- [Installation](#installation)
- [Main Steps](#main-steps)
  - [Initial setup and running a test](#initial-setup-and-running-a-test)
  - [Saving and loading test results](#saving-and-loading-test-results)
  - [Interpreting and viewing test results](#interpreting-and-viewing-test-results)
  - [Optimising the test results](#optimising-the-test-results)
  - [Uploading the settings to the horn](#uploading-the-settings-to-the-horn)
  - [Tidying up and making the horn usable](#tidying-up-and-making-the-horn-usable)
- [Advanced features and debugging](#advanced-features-and-debugging)
- [Troubeshooting](#troubeshooting)
- [Questions that may or may not be asked, let alone frequently asked](#questions-that-may-or-may-not-be-asked-let-alone-frequently-asked)
# Installation
The application requires python 3 along with a few non built-in libraries. Assuming you already have python 3, the libraries can be installed with
```bash
pip3 install numpy sounddevice pyserial piecewise-regression matplotlib
```
Download this repository or at the very least [`BikeHornOptimiser.py`](BikeHornOptimiser.py) and the [corresponding Arduino sketch](OptimiserSketch).

# Main Steps
## Initial setup and running a test
1. Upload the [Optimiser Sketch](OptimiserSketch) to the horn.
2. Find a quiet place to make not very quiet for the next little while.
3. Run [`BikeHornOptimiser.py`](BikeHornOptimiser.py). In the terminal, this will be something like `python3 BikeHornOptimiser.py`. On Windows, this seems to be a little slow in starting. If no window appears, attempt to run the script from the terminal and see if there is any error messages or missing libraries reported. A window should appear looking something like this:  
![Labelled diagram of the bike horn optimiser application, showing the tabbed layout](images/OptimiserLabelledDiagram.png)
4. If you have existing test data with settings you would like to reuse, open this file using the *Save / Load results* tab (see the [Saving and loading test results](#saving-and-loading-test-results).
5. Navigate to the *Run Test / Serial* tab (you will be on it by default). Connect the horn to the computer using a mini usb cable and select its serial port (something like `COM6` on Windows and `/dev/ttyUSB0` or `/dev/ttyACM0` on other operating systems). Try clicking the *Refresh* button if the serial port is not listed.
6. Edit the parameters for which to run the test with. These allow you to specify the ranges of duty cycles to test for driving the piezo siren and driving the voltage boosting stage, as well as the amount to increment these values between each test. You can also select what range of MIDI notes to test and allow the horn to cool down for a few seconds in between testing each MIDI note.
7. When ready, click *Start test*. If it is greyed out, no serial port is selected. Select the horn's serial port and try again. All going well, the horn should start making noises a few seconds later. The message box will print an estimate of the time that the test will take followed by the progress

## Saving and loading test results
Test results can be saved and loaded from a file so that the results can be analysed and uploaded at a different time to when the test is run. In case of crashes or unexpected interruptions when running a test, a backup file is saved after testing each note. This has the same name at the currently opened file (or one based off the time the application was started if nothing was opened) with "BACKUP" appended to the end.

Use the *Open* button to open a file, loading stored settings and test data and the *Save as* button to save all existing settings and test data. Opening a file will clear any existing, unsaved data.

## Interpreting and viewing test results
The *View results* tab contains a 3D surface plot for each note showing the percieved loudness as each of the two parameters of the duty cycle for the piezo element and the boost stage are varied. The optimal settings of these duty cycles are the highest point on the loudnes scale (marked by an orange dot). The scroll bar below the graph can be used to adjust the data for which note is displayed. Some notes will have a nice, clean surface such as this example:  
![Screenshot of the results tab for midi note 90 of a test showing a nice smooth surface](images/ResultsNiceExample.png)  
Other notes appear much noisier in their data. The aim is to reduce this noise as much as possible, although I'm not quite sure how to do this at the moment apart from trying to minimise background noise during the testing process.

The *Optimisation settings* tab shows the settings picked as optimal for each midi note (i.e. the settings for the orange dot in the *View results* tab for each midi note). An example of this is shown below:  
![Screenshot of the Optimisation Settings tab showing the settings chosen as optimal for each midi note](images/BestPicked.png)

## Optimising the test results
Once you have some testing data, navigate to the *Optimisation Settings* tab and click *Optimise*. After a few seconds, a couple of orange lines should appear, courtesy of the [piecewise-regression](https://github.com/chasmani/piecewise-regression/) library. These approximate the measured data. Feel free to play around with the number of breakpoints, set using the number entry boxes below the graph, to produce approximations that most closely match the measured data. An example is shown below:  
![Screenshot of the optimisation window with midi scale and orange lines](images/OptimisedMIDIScale.png)  
Due to some of the randomness involved in fitting lines to the data, for higher numbers of breakpoints, it may take multiple attemts to get the results to "converge" and thus find an acceptable solution, if they do at all.

Internally, the horn uses the maximum value the piezo timer (timer 1) counts to as the x axis instead of the midi note and the compare value for the relevent timer instead of the duty cycle. If you would like to view the data using these scales, tick the *Show timer values* checkbox belows the graphs. An extra green line will be displayed on each graph. This represents the actual linear lines that will be uploaded to the horn, allowing for integer maths and limited space available to store the relevent numbers. An example is shown below:  
![Screenshot of the optimisation window with midi scale and orange and green lines](images/OptimisedTimerScale.png)

## Uploading the settings to the horn
As shown below, the top two text boxes in the *View / Upload optimisations* tab show my attempt at drawing piecewise function representations of the settings that will be uploaded. The final text box shows the raw data that will be uploaded to the EEPROM in the horn, mainly for curiosity's sake.  
![Screenshot of the upload tab showing the text boxes with the piecewise functions and raw data to upload](images/Upload.png)

Make sure the horn is connected and the serial port is set correctly on the *Run test / Serial* tab. Now click the *Upload to horn* button. All going well, the settings will be uploaded to the horn.

## Tidying up and making the horn usable
Flash the main [bike horn sketch](../BikeHorn/) to make the horn usable again and to put it back into power saving mode so that it will not flatten any installed batteries if left sitting for too long.

# Advanced features and debugging
## Previewing settings before uploading them <!-- omit in toc -->
A preview tool has been included to allow easier experimentation with settings before uploading them to the horn's EEPROM. This can be accessed from the *Preview current optimisations on horn* button in the *View / Upload optimisations* tab. This will display a new window as shown below, allowing single notes or a range of midi notes to be played with selectable duty cycles for the piezo element and boost stage.  
![Screenshot of the preview window](images/PreviewWindow.png)

## Dumping the contents of the EEPROM <!-- omit in toc -->
Connect the horn and click the *Dump EEPROM of a connected horn* button in the *Help / About* tab.

## Erasing the EEPROM <!-- omit in toc -->
Connect the horn and click the *Wipe EEPROM of a connected horn* button in the *Help / About* tab.

# Troubeshooting
## The serial port does not appear <!-- omit in toc -->
Check whether other software such as the Arduino IDE can see the serial port. Make sure no other programs have it open or are currently using it.

## "Did not get a response from the horn" message <!-- omit in toc -->
Are you using the correct serial port and is the [Optimiser sketch](OptimiserSketch/) uploaded to the horn? Also make sure no other software is currently using the serial port. If this doesn't work, try unplugging and replugging in the usb cable.

## Arduino not resetting or misbehaving after using this application <!-- omit in toc -->
Try unplugging and replugging in the usb cable.
## Application freezes when trying to close it <!-- omit in toc -->
This is a bug I have not worked out yet - something to do with the audio level monitoring. If you are running the application in a terminal, try using *Control-C* a couple of times. If that doesn't work or you aren't using a terminal, kill the program using the task manager.

# Questions that may or may not be asked, let alone frequently asked
## Why store the optimised settings in EEPROM? <!-- omit in toc -->
The original optimisation scripts outputted C++ code to paste into the [Bike horn Arduino sketch](../BikeHorn). While this does work for one horn, I could see this becoming a problem if more than one horn is ever built, as you would end up with code that needs to be customised for each horn and then kept track of through updates and changing of the installed tunes, making it messy to deal with. Instead, the aim of storing the optimised settings in EEPROM is that the settings live with the horn rather than the source code, meaning that the same source code can theoreticaly be used with every horn in existance and they should all perform optimally as far as selecting the best duty cycles of each stage. Tunes may still need to be transposed to suit each horn's preferred frequency range though.

## Why is the optimising sketch not a part of the main bike horn sketch? <!-- omit in toc -->
The aim of optimising is that it doesn't need to be done all that often. Seeing as the tunes are stored as part of the program, the horn will need to be reprogrammed whenever the installed tunes are uploaded, so it is not much of an extra burden to upload a special optimising sketch. It is also one less thing to potentially go wrong in the main sketch and besides, it allows even more room for tunes 😉.

## Why the limit of 10 lines (9 breakpoints) per parameter to optimise? <!-- omit in toc -->
The processed optimisations are stored in EEPROM in the Arduino microcontroller on the horn. This is limited to 1024 bytes on an Arduino Nano. Each linear function takes up 8 bytes. Because (as an optional feature) the EEPROM is also used for tracking the number of times and for how long the horn is used for battery life monitoring, this resource needs to be shared. Feel free to adjust the parameters for the amount and addresses to allocate in the source code of this optimiser and 'defines.h' of the main bike horn sketch, making sure that they are in agreement for things to work as expected.

## Why did the battery go flat after optimising? <!-- omit in toc -->
Unlike the main bike horn sketch, she optimiser sketch does not contain any power saving features. Thus, when taking a break or leaving the horn sit, make sure to reflash the main [bike horn sketch](../BikeHorn) or take the batteries out.

## Why is the lowest midi note 23? <!-- omit in toc -->
This is due to the internal hardware and setup of the timers in the horn. Timer 1 is used to provide the frequency of the note to play by starting from 0 and counting up to a specified number before resetting back to 0. The time for this to happen is the period of the wave and thus the lower the note, the larger this number must be. Timer 1 is a 16 bit timer, meaning the maximum value it can count up to is *2^16-1 = 65535*, meaning that without mucking around with prescalars and clock speeds, midi note 23 is the lowest note where the value to count to is below 65535.

Note that this is a very low note for a piezo siren and is more a series of clicks anyway - not much use for a horn, let alone anything lower.