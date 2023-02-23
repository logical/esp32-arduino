# esp32-audio
fsk_mod_demod

this code is for a basic rtty receiver transmitter. It is a little unstable but it will  transmit and receive a few characters.
 
 work in progress.
inmp441 microphone
MAX98357A audio amplifier

use minimodem to test.


minimodem --tx --baudot --mark 2200 --space 1200 --startbits 1 --stopbits 1.5 200


trying to get to 300 baud but it would probably be better in hardware.


I think that ADC 1 is not affected by wifi only ADC 2 so I will only use ADC 1, so I can add wifi.
DAC or ADC can be controlled by I2S0 only so you can't add both on I2S.
