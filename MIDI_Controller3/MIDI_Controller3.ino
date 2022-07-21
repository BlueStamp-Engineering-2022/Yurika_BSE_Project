/*
  
     >> Making Music with Arduino: https://nerdmusician.teachable.com/p/making-music-with-arduino
     >> Course Making Music with Arduino in Portuguese: https://www.musiconerd.com/curso-completo

  By by Gustavo Silveira, 2020.
  - This Sketch reads Arduino's digital and analog ports and sends midi notes and MIDI Control Change

  Want to learn how to make your own code and understand other people's code?
  Check out our complete course on Making Music with Arduino: http://musiconerd.com/curso-completo
  
  http://www.musiconerd.com
  http://www.youtube.com/musiconerd
  http://facebook.com/musiconerdmusiconerd
  http://instagram.com/musiconerd/
  http://www.gustavosilveira.net
  gustavosilveira@musiconerd.com

*/


///////////////////////////////////////////
// choosing your board
// Set your board, choose:
// "ATMEGA328" if using ATmega328 - Uno, Mega, Nano...
// "ATMEGA32U4" if using with ATmega32U4 - Micro, Pro Micro, Leonardo...
// "TEENSY" if using a Teensy card
// "DEBUG" if you just want to debug the code on the serial monitor
// you don't need to comment or uncomment any MIDI library below after defining your board

#define ATMEGA32U4 1 //* put here the uC you are using, like in the lines above followed by "1", like "ATMEGA328 1", "DEBUG 1", etc.

///////////////////////////////////////////
// LIBRARIES
// -- Define the MIDI library -- //

// if using with ATmega328 - Uno, Mega, Nano ...
#ifdef ATMEGA328
#include <MIDI.h>
// MIDI_CREATE_DEFAULT_INSTANCE();

// if using with ATmega32U4 - Micro, Pro Micro, Leonardo...
#elif ATMEGA32U4
#include "MIDIUSB.h"

#endif
// ---- //

///////////////////////////////////////////
// BUTTONS
const int N_BUTTONS = 12; //* total number of buttons
const int BUTTON_ARDUINO_PIN[N_BUTTONS] = {2,3,4,5,9,8,7,6,10, 11,12,13}; //* pins of each button connected directly to the Arduino

//#define pin13 1 // uncomment if you are using pin 13 (the led pin), or comment out the line if not
//byte pin13index = 12; //* put index of pin 13 of buttonPin[] array if you are using it, if not, comment

int buttonCState[N_BUTTONS] = {}; // stores the current value of the button
int buttonPState[N_BUTTONS] = {}; // stores the previous value of the button

// debounce
unsigned long lastDebounceTime[N_BUTTONS] = {0}; // the last time the output pin was toggled
unsigned long debounceDelay = 20; //* the debounce time; increase if the output is sending too many notes at once

///////////////////////////////////////////
// POTENTIOMETERS
const int N_POTS = 4; //* total number of pots (slide and rotary)
const int POT_ARDUINO_PIN[N_POTS] = {A3, A2, A1, A0}; //* pins of each pot connected directly to the Arduino

int potCState[N_POTS] = {0}; // current state of the analog port
int potPState[N_POTS] = {0}; // previous state of the analog port
int potVar = 0; // variation between the value of the previous and current state of the analog port

int midiCState[N_POTS] = {0}; // Current state of midi value
int midiPState[N_POTS] = {0}; // Previous state of midi value

const int TIMEOUT = 300; //* amount of time the potentiometer will be read after exceeding the varThreshold
const int varThreshold = 10; //* threshold for the variation in the potentiometer signal
boolean potMoving = true; // if the potentiometer is moving
unsigned long PTime[N_POTS] = {0}; // previously stored time
unsigned long timer[N_POTS] = {0}; // stores the time that has passed since the timer was reset

///////////////////////////////////////////
// midi
byte midiCh = 2; //* MIDI channel to be used
byte note = 36; //* lowest grade to use
byte cc = 11; //* Lowest MIDI CC to use

///////////////////////////////////////////
// SETUP
void setup() {

  // Baud Rate
  // use if using with ATmega328 (uno, mega, nano...)
  // 31250 for MIDI class compliant | 115200 for Hairless MIDI
 // Serial.begin(115200); //*

#ifdef DEBUG
Serial.println("Debug mode");
Serial.println();
#endif

  // Buttons
  // Initialize buttons with pull up resistor
  for (int i = 0; i < N_BUTTONS; i++) {
    pinMode(BUTTON_ARDUINO_PIN[i], INPUT_PULLUP);
  }

//#ifdef pin13 // initialize pin 13 as an input
//pinMode(BUTTON_ARDUINO_PIN[pin13index], INPUT);
//#endif


}

///////////////////////////////////////////
// LOOP
void loop() {

   buttons();
   potentiometers();

}

///////////////////////////////////////////
// BUTTONS
void buttons() {

   for (int i = 0; i < N_BUTTONS; i++) {

     buttonCState[i] = digitalRead(BUTTON_ARDUINO_PIN[i]); // read arduino pins

//#ifdef pin13
//if (i == pin13index) {
//buttonCState[i] = !buttonCState[i]; // reverse pin 13 because it has a pull down resistor instead of a pull up
//}
//#endif

     if ((millis() - lastDebounceTime[i]) > debounceDelay) {

       if (buttonPState[i] != buttonCState[i]) {
         lastDebounceTime[i] = millis();

         if (buttonCState[i] == LOW) {

           // Send the MIDI note according to the chosen board
#ifdef ATMEGA328
// ATmega328 (uno, mega, nano...)
MIDI.sendNoteOn(note + i, 127, midiCh); // note, velocity, channel

#elif ATMEGA32U4
// ATmega32U4 (micro, pro micro, leonardo...)
noteOn(midiCh, note + i, 127);  // channel, note, velocity
MidiUSB.flush();

#elif TEENSY
// Teensy
usbMIDI.sendNoteOn(note + i, 127, midiCh); // note, velocity, channel

#elif DEBUG
Serial.print(i);
Serial.println(": button on");
#endif

         }
         else {

           // Send the MIDI OFF note according to the chosen board
#ifdef ATMEGA328
// ATmega328 (uno, mega, nano...)
MIDI.sendNoteOn(note + i, 0, midiCh); // note, velocity, channel

#elif ATMEGA32U4
// ATmega32U4 (micro, pro micro, leonardo...)
noteOn(midiCh, note + i, 0);  // channel, note, velocity
MidiUSB.flush();

#elif TEENSY
// Teensy
usbMIDI.sendNoteOn(note + i, 0, midiCh); // note, velocity, channel

#elif DEBUG
Serial.print(i);
Serial.println(": button off");
#endif

        }
        buttonPState[i] = buttonCState[i];
      }
    }
  }
}

/////////////////////////////////////////////
// POTENTIOMETERS
void potentiometers() {

  /* so that only the analog ports are read when they are used, without losing resolution,
    It is necessary to establish a "threshold" (varThreshold), a minimum value that the ports have to be moved
    to start reading. After that, a kind of "gate" is created, a gate that opens and allows
    that the analog ports are read without interruption for a certain time (TIMEOUT). When the timer is less than TIMEOUT
    means that the potentiometer was moved very recently, which means that it is probably still moving,
    so the "gate" must be kept open; if the timer is greater than TIMEOUT, it means that it has not been moved for a while,
    then the gate must be closed. For this logic to happen, the timer (lines 99 and 100) must be reset each time the analog port
    vary more than the varThreshold set.
  */


  // Debug only
  // for (int i = 0; i < nPots; i++) {
  // Serial.print(potCState[i]); Serial.print(" ");
  // }
  // Serial.println();

  for (int i = 0; i < N_POTS; i++) { // Loop all pots

    potCState[i] = analogRead(POT_ARDUINO_PIN[i]);
    Serial.println(i);
    midiCState[i] = map(potCState[i], 0, 1023, 0, 127); // Map the read from potCState to a usable value in midi

    potVar = abs(potCState[i] - potPState[i]); // Calculate the absolute value between the difference between the current and previous state of the pot

    if (potVar > varThreshold) { // Open the gate if the potentiometer variation is greater than the threshold (varThreshold)
      PTime[i] = millis(); // Store the previous time
    }

    timer[i] = millis() - PTime[i]; // Reset the timer 11000 - 11000 = 0ms

    if (timer[i] < TIMEOUT) { // If the timer is less than the maximum time allowed, it means the potentiometer is still moving
      potMoving = true;
    }
    else {
      potMoving = false;
    }

    if (potMoving == true) { // If the pot is still moving, send control change
      if (midiPState[i] != midiCState[i]) {

        // Send MIDI CC according to the chosen board
#ifdef ATMEGA328
// ATmega328 (uno, mega, nano...)
MIDI.sendControlChange(cc + i, midiCState[i], midiCh); // cc number, cc value, midi channel

#elif ATMEGA32U4
// ATmega32U4 (micro, pro micro, leonardo...)
controlChange(midiCh, cc + i, midiCState[i]); // (channel, CC number, CC value)
MidiUSB.flush();

#elif TEENSY
// Teensy
usbMIDI.sendControlChange(cc + i, midiCState[i], midiCh); // cc number, cc value, midi channel

#elif DEBUG
Serial.print("Pot: ");
Serial.print(i);
Serial.print(" ");
Serial.println(midiCState[i]);
//Serial.print(" ");
#endif

        potPState[i] = potCState[i]; // Store current potentiometer reading to compare with next
        midiPState[i] = midiCState[i];
      }
    }
  }
}

///////////////////////////////////////////
// if using with ATmega32U4 (micro, pro micro, leonardo...)
#ifdef ATMEGA32U4

// Arduino (pro)micro midi functions MIDIUSB Library
void noteOn(byte channel, byte pitch, byte velocity) {
   midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
   MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
   midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
   MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value) {
   midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
   MidiUSB.sendMIDI(event);
}

#endif
