# Getting started
To get started and developing on this project, simply download the code from here, copy the content of the template-env.ini file into a new file and call it env.ini. Then fill out the variables like wifi name and password, your mqtt broker endpoint, ensure your certs.h is correct, and you're good to go!

This project is setup to remotely update the esp8266. It does this by utilizing the ArduinoOTA library.

The relevant code for development is all in /src, and the main entry point is the file main.cpp.

Table of content:
1. [Introduction](#introduction)
2. [main.cpp](#maincpp)
3. [ledstrip.cpp](#ledstripcpp)
4. [ledmodes](#ledmodes)
5. [improvements](#improvements)


## Introduction
This project contains the code for handling 2 led strips (or any amount with some tweaking), and with it follows some basic lighting effects which can be freely chosen and swapped between. As a standard there is also a rotating mode which switches between two given modes. This can be controlled by the ```InitializeLEDs()``` method in main.

## main.cpp
The two most interesting parts of main.cpp is the setup and loop functions. Setup runs once on startup, and thereafter loop runs continuously.

In setup you want to ensure that you initialize your leds, connect to the internet and setup your MQTT broker. It is also where the ArduinoOTA setup happens, which is what allows the wireless updates.

In loop everything else happens. Here we connect to the MQTT broker if not already connected, listen to the broker for new messages, listen after any new wireless updates, give time for the esp to handle its own processes, and update the leds.

## ledstrip.cpp
```ledstrip.cpp``` exports a class, LEDStrip, which is a full controller over the ledstrip. The constructor takes two arguments, a CRGB variable (often array) and an integer, dictating respectively the array which holds the data for the leds and the amount of led lights in total. Then there is the loop function which gets called in ```loop()``` in ```main.cpp```. This function simply checks continuously if the right effect is being shown, and if not, resets and updates. There shouldn't be any need to change something in this function. The rest of the functions are helper functions to update state, so they shouldn't be touched either.

The exception here is if you want to add a new effect or similar, then you should add the mode to ```parseMode``` in the .cpp file, and update the enum in the .h file. In the .h file you can find all the variables which control the ledstrip, including loop_speed, LED_mode, progress (which ties entirely to mode_progress_bar, being the value between 0-100), and more.

## ledmodes
In this folder, /ledmodes, you can find all the modes/effects currently available for the ledstrips. To add a new ledmode, simply add corresponding name_effect.cpp and name_effect.h files, and add them in, 1) effect.cpp, 2) led_strip.h (enum), and 3) led_strip.cpp (parseMode). To get a good idea of how to create/structure an effect, look at both solid_effect and rainbow_effect, as that should give you a good understanding of how the implementation works.


## improvements
Suggestions and feedback are both greatly apprectiated.

