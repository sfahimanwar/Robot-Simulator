Sheikh Fahim Anwar
101141744

Source Files - 
display.c - contains the window/drawing code to display the robots
environmentServer.c - code that runs the environment server
robotClient.c - code for the simulated robot
stop.c - code for a process that will stop the simulator

Header files - 
simulator.h - contains definitions and structs that will be use throughout the code

Makefiles - 
makefile - use this to compile

This is a robot simulation program. These simple robots use multiple threads and allows multiple robots to connect to it with each robot running as its own process.

Program can only be run in linux environment as it uses -lX11 library, all dependencies and libraries are set up in the makefile.


