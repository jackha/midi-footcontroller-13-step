# 13 STEP midi footcontroller
A diy melodic midi footcontroller using 3d printing and arduino

## Features
- Plays 13 notes (1 octave + 1) through a din5 midi output
- MIDI port, velocity and other defaults are defined in the source code to keep everything simple 

## Hardware
- Pro Trinket 5v (I'm using 3v, that works as well, but it's not "ideal" for 8x8 matrix... but it works!)
- Adafruit PowerBoost 1000c
- LiPo 3.7v 2500mAh battery, fits in 50mm x 62mm
- Adafruit 8x8 0.8" matrix

## The circuit
  - digital in 1 connected to MIDI jack pin 5
  - MIDI jack pin 2 connected to ground
  - MIDI jack pin 4 connected to +5V through 220 ohm resistor
  - Attach a MIDI cable to the jack, then to a MIDI synth, and play music.
 
  A4: 8x8 data
  A5: 8x8 clock

  Buttons: see swPins[]
  
  Special buttons are set in the special buttons section
  
  The program uses a simple debouncing algorithm that disables an input for a certain time (depending on press or depress) when a change is detected.
  
## Credits  
  MIDI code based on example code by Tom Igoe
  http://www.arduino.cc/en/Tutorial/Midi
  
  8x8 matrix code: based on Adafruit 8x8 example
