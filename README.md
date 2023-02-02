# esp32-rtty
esp32 rtty
 this code is for a basic rtty receiver transmitter. It is a little unstable but it will  transmit and receive a few characters.
 
 work in progress.
inmp441 microphone
MAX98357A audio amplifier


I used i2s because the adc only works at 1khz when the wifi is on and the dac needed an amplifier anyway. it uses a zero crossing
detector to determine the audio frequency.


use minimodem to test.
minimodem --tx --baudot  --mark 2200 --space 1200 rtty


trying to get to 300 baud but it would probably be better in hardware.
