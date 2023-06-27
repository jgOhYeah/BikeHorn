# Makefile
# Commands for arcuino-cli. This is a bit slower than the Arduino IDE, but has
# prettier output and can be run from the inbuilt terminal easily.
#
# Written by Jotham Gates
# Last modified 01/11/2022

%phony: compile upload

compile:
	arduino-cli compile --fqbn arduino:avr:uno

upload:
	arduino-cli upload --fqbn arduino:avr:uno -p /dev/ttyUSB0