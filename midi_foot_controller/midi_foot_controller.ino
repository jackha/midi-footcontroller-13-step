/*
  MIDI foot controller, 
  
  MIDI code based on example code by Tom Igoe
  http://www.arduino.cc/en/Tutorial/Midi
  
  8x8 matrix code: based on Adafruit 8x8 example
  

  Pro Trinket 5v (I'm using 3v, that works as well, but it's not "ideal" for 8x8 matrix... but it works!)

  The circuit:
  - digital in 1 connected to MIDI jack pin 5
  - MIDI jack pin 2 connected to ground
  - MIDI jack pin 4 connected to +5V through 220 ohm resistor
  - Attach a MIDI cable to the jack, then to a MIDI synth, and play music.
  
  
  A4: 8x8 data
  A5: 8x8 clock

  Buttons: see swPins[]
  They by default trigger a note on and note off, see notes[i]
  
  Special buttons are set in the special buttons section
  
  The program uses a simple debouncing algorithm that disables an input for a certain time (depending on press or depress) when a change is detected.
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

Adafruit_8x8matrix matrix = Adafruit_8x8matrix();

#define midiChannel 6
const byte velocity=0x45;

#define settingModePressTime 3000

#define numButtons 14
#define pressDebounceDelay 10  // time in ms after a press that we don't read new values
#define depressDebounceDelay 50

#define screenMessageTime 1000  // in ms

#define modeNote 0
#define modeSettings 1

// special buttons
#define settingButtonIndex 6  // points to swPins[index], notes[index] is then not used

// in setting mode
#define settingModeOctaveUpIndex 3
#define settingModeOctaveDownIndex 1
#define settingModeHoldIndex 2
#define settingModeAuditionIndex 0  // to verify your octave

int swPins[] = {3,4,5,6,8,9,A0,10,11,12,13,A1,A2,A3};  // A4 and A5 are used for the 8x8 matrix display, 13 interferes with 4???
int notes[] = {60,61,62,63,64,65,0,66,67,68,69,70,71,72}; 
int swCurrent[numButtons];
unsigned long timeStartReading[numButtons];
unsigned long timeNow;

int lastNote = 0;  // used for releasing last note in "hold" function
bool holdLastNote = true;

int transpose = 0;
int i = 0;
int mode = modeNote;

bool screenNeedsClear = true;
unsigned long clearScreenTime;

static const uint8_t PROGMEM
  start_bmp[] =
  { 
    B01101110,
    B11100011,
    B01100011,
    B01100110,
    B01100011,
    B01100011,
    B01100011,
    B11101110
  },
  note_mode_bmp[] =
  {
    B00000110,
    B00000110,
    B00000110,
    B00000110,
    B00111110,
    B01111110,
    B01111110,
    B00111100
  },
  setting_mode_bmp[] =
  {
    B00011000,
    B01011010,
    B00111100,
    B11111111,
    B11111111,
    B00111100,
    B01011010,
    B00011000
  },
  notes_bmp[][8] =
  {
    {
    B01100000,
    B10000000,
    B10000000,
    B10000000,
    B01100000,
    B00000000,
    B00000000,
    B00000000
    },
    {
    B01100000,
    B10000100,
    B10001110,
    B10000100,
    B01100000,
    B00000000,
    B00000000,
    B00000000
    },
    {
    B11000000,
    B10100000,
    B10100000,
    B10100000,
    B11000000,
    B00000000,
    B00000000,
    B00000000
    },
    {
    B11000000,
    B10100100,
    B10101110,
    B10100100,
    B11000000,
    B00000000,
    B00000000,
    B00000000
    },
    {
    B11100000,
    B10000000,
    B11100000,
    B10000000,
    B11100000,
    B00000000,
    B00000000,
    B00000000
    },
    {
    B11100000,
    B10000000,
    B11100000,
    B10000000,
    B10000000,
    B00000000,
    B00000000,
    B00000000
    },
    {
    B11100000,
    B10000100,
    B11101110,
    B10000100,
    B10000000,
    B00000000,
    B00000000,
    B00000000
    },
    {
    B01100000,
    B10000000,
    B10100000,
    B10100000,
    B01100000,
    B00000000,
    B00000000,
    B00000000
    },
    {
    B01100000,
    B10000100,
    B10101110,
    B10100100,
    B01100000,
    B00000000,
    B00000000,
    B00000000
    },
    {
    B01000000,
    B10100000,
    B11100000,
    B10100000,
    B10100000,
    B00000000,
    B00000000,
    B00000000
    },
    {
    B01000000,
    B10100100,
    B11101110,
    B10100100,
    B10100000,
    B00000000,
    B00000000,
    B00000000
    },
    {
    B11000000,
    B10100000,
    B11100000,
    B10100000,
    B11000000,
    B00000000,
    B00000000,
    B00000000
    },
  },
  transpose_bmp[][8] =
  {
    {
    B11000011,
    B01100110,
    B00111100,
    B00011000,
    B11000011,
    B01100110,
    B00111100,
    B00011000
    },
    {
    B00000000,
    B00000000,
    B11000011,
    B01100110,
    B00111100,
    B00011000,
    B00000000,
    B00000000
    },
    {
    B00000000,
    B00011000,
    B00100100,
    B01000010,
    B01000010,
    B00100100,
    B00011000,
    B00000000
    },
    {
    B00000000,
    B00000000,
    B00011000,
    B00111100,
    B01100110,
    B11000011,
    B00000000,
    B00000000
    },
    {
    B00011000,
    B00111100,
    B01100110,
    B11000011,
    B00011000,
    B00111100,
    B01100110,
    B11000011
    },
  },
  hold_bmp[] =
  {
    B10101110,
    B11101010,
    B10101110,
    B00000000,
    B10001100,
    B10001010,
    B11101100,
    B00000000
  },
  nohold_bmp[] =
  {
    B11001110,
    B10101010,
    B10101110,
    B00000000,
    B10101100,
    B11101010,
    B10101100,
    B00000000
  },
  template_bmp[] =
  {
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B00000000
  };

void setup() {
  // 8x8
 
  matrix.begin(0x71);  // default address is 0x70. 
  matrix.clear();
  matrix.drawBitmap(0, 0, start_bmp, 8, 8, LED_ON);
  matrix.writeDisplay();
  screenNeedsClear = true;
  clearScreenTime = millis() + screenMessageTime;
  mode = modeNote;
  
  // control inputs
  pinMode(LED_BUILTIN, OUTPUT);
  // init buttons
  for (int i=0; i<numButtons; i++) {
    pinMode(swPins[i], INPUT_PULLUP);
    swCurrent[i] = HIGH;
    timeStartReading[i] = 0;
  }
  // Set MIDI baud rate:
  Serial.begin(31250);
}

// plays a MIDI note. Doesn't check to see that cmd is greater than 127, or that
// data values are less than 127:
void noteOn(int cmd, int pitch, int velocity) {
  Serial.write(cmd);
  Serial.write(pitch);
  Serial.write(velocity);
}

void matrixImage(const uint8_t image[])
{
  matrix.clear();
  matrix.drawBitmap(0, 0, image, 8, 8, LED_ON);
  matrix.writeDisplay();  
  screenNeedsClear = false;
}

void matrixClear(unsigned long time)
{
  screenNeedsClear = true;
  clearScreenTime = time;
}

void loop() {

  i = (i + 1) % numButtons;
  
  timeNow = millis();
  if (timeNow > timeStartReading[i])
  {  
    if (digitalRead(swPins[i]) != swCurrent[i])
    {
      if (swCurrent[i] == HIGH) 
      {
        // press the button
        swCurrent[i] = LOW;
        timeStartReading[i] = timeNow + pressDebounceDelay;
        switch (i) 
        {
          case settingButtonIndex:
            noteOn(0x8F+midiChannel, lastNote, 0x00);  // kill last note
            lastNote = -1;
            break;
          default:
            switch(mode)
            {
              case modeNote:
                if (holdLastNote) 
                {
                  noteOn(0x8F+midiChannel, lastNote, 0x00);  // kill last note
                }
                noteOn(0x8F+midiChannel, notes[i]+transpose, velocity);
                lastNote = notes[i]+transpose;
                matrixImage(notes_bmp[notes[i] % 12]);
                if (!holdLastNote)
                {
                  matrixClear(timeNow + screenMessageTime);
                }
                break;
              case modeSettings:
                switch (i)
                {
                  case settingModeAuditionIndex:              
                    noteOn(0x8F+midiChannel, notes[i]+transpose, velocity);
                    break;
                  default:
                    break;
                }
                break;
            }
        }
      } else {
        // depress the button
        swCurrent[i] = HIGH;
        // set timeStartReading after the switch as we use it to see when it last changed as well.
        //timeStartReading[i] = timeNow + depressDebounceDelay;
        // Every index that is not one of the special buttons is defined as a chromatic/note button
        switch (i) 
        {
          case settingButtonIndex:
              switch(mode)
              {
                case modeNote:
                  if (timeNow - timeStartReading[i] > settingModePressTime)  // only switch mode if we pressed the button for more than 5000 + debounce delay time.
                  {
                    mode = modeSettings;
                    matrixImage(setting_mode_bmp);
                  }
                  break;
                case modeSettings:
                  mode = modeNote;
                  matrixImage(note_mode_bmp);
                  matrixClear(timeNow + screenMessageTime);
                  break;
            }
            break;
          default:
            if (!holdLastNote)
            {
              noteOn(0x8F+midiChannel, notes[i]+transpose, 0x00); // slightly less chance of hanging notes
            }
            switch(mode) {
              case modeSettings:
                switch(i) {
                  case settingModeOctaveUpIndex:
                    if (transpose < 12) 
                    {
                      transpose += 12;
                    }
                    matrixImage(transpose_bmp[(transpose / 12) + 3]);  // convert to index 0..4
                    break;
                  case settingModeOctaveDownIndex:
                    if (transpose > -36) 
                    {
                      transpose -= 12;
                    }
                    matrixImage(transpose_bmp[(transpose / 12) + 3]);  // convert to index 0..4
                    break;
                  case settingModeHoldIndex:
                    holdLastNote = !holdLastNote;
                    holdLastNote ? matrixImage(hold_bmp) : matrixImage(nohold_bmp);
                    break;
                  case settingModeAuditionIndex:
                    noteOn(0x8F+midiChannel, notes[i]+transpose, 0x00);
                    break;
                  default:
                    //// every non assigned button returns you to note mode
                    //mode = modeNote;
                    //matrixImage(note_mode_bmp);
                    //matrixClear(timeNow + screenMessageTime);
                    break;
                }
                break;
              default:
                break;
            }
            break;
        }
        timeStartReading[i] = timeNow + depressDebounceDelay;
      }
    }
  }
  
  // "progress indicator" for settings button
  if ((i == settingButtonIndex) && (swCurrent[settingButtonIndex] == LOW) && (mode == modeNote))
  {
    int y = (timeNow - timeStartReading[settingButtonIndex]) * 8 / settingModePressTime;
    if (y <= 7) 
    {
      matrix.clear();
      matrix.drawLine(timeNow % 8,7-y, timeNow % 8,7-y, LED_ON);
      matrix.writeDisplay();  // write the changes we just made to the display
    } else {
      matrixImage(setting_mode_bmp);  // show that we're already going to enter setting mode
    }
    matrixClear(0);
  }

  // clear screen if necessary
  if (screenNeedsClear && (timeNow > clearScreenTime))
  {
    screenNeedsClear = false;
    matrix.clear();
    matrix.writeDisplay();
    matrixClear(timeNow + screenMessageTime); // in case you stopped pressing the button before entering settings mode
  }

  // debugging
  //digitalWrite(LED_BUILTIN, swCurrent[1]);
}


