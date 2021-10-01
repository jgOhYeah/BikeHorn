# BikeHorn
A "musical" siren for bicycles.
<img alt="Overview photo of the horn from an angle" src="Images/Overview_NB_Small.png" width="200px" align="right">

## Main steps for building and usage
1. Modify the Arduino nano for low power and low voltage usage.
2. Build the circuit - see [BikeHornPCB](BikeHornPCB) for a PCB design. The siren is from an old smoke alarm.
3. 3D print the case.
4. Optimise parameters for each note to be the loudest possible - see [Tuning](Tuning) for details.
5. Generate the tunes to play using [this Musescore plugin](https://github.com/jgOhYeah/TunePlayer/blob/main/extras/MusescorePlugin.md). Paste this into `tunes.h` in the main [BikeHorn sketch](BikeHorn).
6. Upload to the Arduino and test.
7. Mount on bicycle / vehicle and have fun.

A low resolution gif that gives an idea of order of assembly:

![AssemblyGIF.gif](Images/AssemblyGIF.gif)
