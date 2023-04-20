# esp32-arduino

My projects for esp32.

### fsk_mod_demod 

this code is for a basic rtty receiver transmitter. It is a little unstable but it will  transmit and receive a few characters.
It uses i2s microphone and speaker.inmp441 microphone.MAX98357A audio amplifier


### fsk_mod_demod_experimental 

uses wired connection. uses i2s receiver and tone to transmit using a square wave. 1200bps, but has very bad reception. About 80% . I am looking for suggestions on how to get the reliability up. Not sure yet what to do from here . Maybe back off the baud rate to build reliability if that is possible or add error correction.


use minimodem to test.


minimodem --tx --baudot --mark 2200 --space 1200 --startbits 1 --stopbits 1.5 200


minimodem --tx --ascii  --mark 1200 --space 2200 --startbits 1 --stopbits 2  1200.00


I think that ADC 1 is not affected by wifi only ADC 2 so I will only use ADC 1, so I can add wifi.
DAC or ADC can be controlled by I2S0 only so you can't add both on I2S. I have not tested with wifi.

### ADC_GRAPH 

is a smooth scrolling web graph with no flicker.

### I2S_WEB_MIC

A web server streaming microphone example.

### SIMPLE_DAC

My simple example of audio using the dac.

### cameraWebserverSmall

A stripped down version of the esp32cam example file for ai-thinker only
