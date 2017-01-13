# README

This is my attempt at running two X-Nucleo-IHM01A1 shields (stepper motor controllers) on a STM32 Nucleo-F411RE evaluation board, using plain C and libopencm3.

Currently the code receives simple gcode commands (one command per line) from USART2, which is on Nucleo boards presents a terminal emulator on the computer. The gcode parser is implemented using ragel. Only moves in lines, no arcs.

Example command: 
<code>G00 X2000 Y-3000 F800</code>
runs the x-axis 2000 steps forward and the y-axis 3000 steps backwards, at combined speed 800 steps / unit time.

Feel free to use my code as you see fit. This is a work in progress.

## Unimplemented and TODO

* Acceleration and deceleration
* Check the appropriate scalings and whatnot, so that speed would be in steps/second
* Moar error checking
* Moar refactoring

## Board connections

* Nucleo F411RE
* Two X-NUCLEO IHM01A1 shields, properly configured with the solder bridges
* Two stepper motors
* Compatible power supply or supplies

