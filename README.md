# esp32-arduino

My projects for esp32.
disclaimer:
When I say projects I mean experiments. some of them don't really work ,yet.

### fsk_mod_demod 

this code is for a basic rtty receiver transmitter. It is a little unstable but it will  transmit and receive a few characters.
It uses i2s microphone and speaker.inmp441 microphone.MAX98357A audio amplifier

minimodem --tx --baudot --mark 2200 --space 1200 --startbits 1 --stopbits 1.5 200


### fsk_mod_demod_experimental 

uses wired connection. uses i2s receiver and tone to transmit using a square wave. 1200bps, but has very bad reception. About 90% . I am looking for suggestions on how to get the reliability up. Not sure yet what to do from here . Maybe back off the baud rate to build reliability if that is possible or add error correction.FSK has very bad timing characteristics. if you lose synchronization it messes up the rest of the stream. the timing functions in ESP32 IDF are broken below 1ms, so that sucks.

If I figure the timing out and increase the mark and space frequencies, I think the new receive function might go up to 2400 baud!

use minimodem to test.

minimodem --tx --ascii  --mark 1200 --space 2200 --startbits 1 --stopbits 2  1200

#### GETTING THE FILTER COEFFIENTS USING GNU OCTAVE

```
Fs = 19200;
freq1 = 2200;
freq2 = 1200;
intvl = 1/Fs;
secs = 1/freq2;
tim = 0 : intvl : secs-intvl;

wavehisin = sin(tim*2*pi*freq1);
wavehicos = cos(tim *2*pi*freq1);
wavelosin = sin(tim*2*pi*freq2);
wavelocos = cos(tim *2*pi*freq2);

disp(wavehisin);
disp(wavehicos);
disp(wavelosin);
disp(wavelocos);
```



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

### serialWifiLogin.ino

get the ssid and password from serial and save them to eeprom.

### FMTX library

Modified copyrighted code from elechouse kt0803 fm transmitter for esp32.

### encoderMenu.ino

This is an example menu using a rotary encoder and a 1306 lcd.

### bluetoothSpeaker.ino

bluetooth speaker.

### serverSD.ino

serve web pages from an sd card.flat directory only with png and jpg images.
