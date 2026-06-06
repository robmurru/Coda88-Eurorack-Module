/*
 
    Coda 88 is a MIDI sound module with three layer 
    of dynamics for full 88 keys master keyboards
 
    https://github.com/sandrolab/coda-88

    Author: 
    Sandro L'Abbate
    https://sandrolabbate.com

    Coding support:
    Alfredo Ardia 
    https://alfredoardia.com

    WAV Trigger:
    https://robertsonics.com/wav-trigger/

    Eurorack Module Version my robmurru 
    https://github.com/robmurru/Coda-88-Eurorack-Module

    Notes:
    - Removed onboard amp (line out output for eurorack usage)
    - Removed release and damper pedal sounds
    - 6 instruments: Grand Piano, DX7 Electric Piano 1, Fender Rhodes, Wurlitzer, Korg M1 Organ 2, Pink Floyd Echoes Piano 
    - Added switch for instrument selection
    - Powered by eurorack 10-pin connector (no need for external DC-DC converter)
  
*/

#include <MIDI.h>
#include <Metro.h>
#include <AltSoftSerial.h>
#include <wavTrigger.h>

// SAMPLES POSITION
#define layerMedio 88
#define layerForte 176
#define layerRelease 264
#define firstRel 285
#define lastRel 372
#define pedalDown 373
#define pedalUp 374
// Dampers range (90 = G6)
#define dampEnd 90

// VELOCITY
#define velMinPiano 0
#define velMaxPiano 55
#define velMinMedio 55
#define velMaxMedio 90
#define velMinForte 90
#define velMaxForte 127

// GAIN
// -70 to +4
#define gainMaster 0
// -70 to +10
#define gainMinPiano -30
#define gainMaxPiano -20
#define gainMinMedio -20
#define gainMaxMedio -5
#define gainMinForte -5
#define gainMaxForte 0

#define AfterPedalLength 16

//EURORACK MODULE
#define BUTTON_PIN 12  //
#define NUM_LEDS 6

const int ledPins[NUM_LEDS] = { A0, A1, A2, A3, A4, A5 };  // LEDs pins
int instSel = 0;
bool buttonClicked = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
int instOffset;  //Offset for selecting the sample bank

#define MIDI_CHANNEL 8  //Put midi channel here

byte DamperState;
int AfterPedal[AfterPedalLength];
int KeyCount = 0;

MIDI_CREATE_DEFAULT_INSTANCE();
wavTrigger wTrig;  // Our WAV Trigger object

void setup() {

  MIDI.begin(MIDI_CHANNEL);
  MIDI.setHandleNoteOn(KeyOn);
  MIDI.setHandleNoteOff(KeyOff);
  MIDI.setHandleControlChange(Sustain);

  wTrig.start();  // WAV Trigger startup at 57600
  wTrig.masterGain(gainMaster);

  //Eurorack instrument selection initialization

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
  updateLEDs();
}

void KeyOn(byte ch, byte note, byte vel) {

  int gain;

  // Piano
  if (vel < velMaxPiano) {
    gain = map(vel, velMinPiano, velMaxPiano, gainMinPiano, gainMaxPiano);
    wTrig.trackGain(note + instOffset, gain);
    wTrig.trackPlayPoly(note + instOffset);
  }

  // Medio
  else if (vel >= velMinMedio && vel < velMaxMedio) {
    gain = map(vel, velMinMedio, velMaxMedio, gainMinMedio, gainMaxMedio);
    wTrig.trackGain((note + instOffset + layerMedio), gain);
    wTrig.trackPlayPoly(note + instOffset + layerMedio);
  }

  // Forte
  else {
    gain = map(vel, velMinForte, velMaxForte, gainMinForte, gainMaxForte);
    wTrig.trackGain((note + instOffset + layerForte), gain);
    wTrig.trackPlayPoly(note + instOffset + layerForte);
  }

  // Tenuto
  for (int i = 0; i < AfterPedalLength; i++) {
    if (AfterPedal[i] == note + instOffset) AfterPedal[i] = 0;
  }
}

void KeyOff(byte ch, byte noteoff, byte vel) {

  if (noteoff <= dampEnd) {
    switch (DamperState) {
      case 0:
        wTrig.trackStop(noteoff + instOffset);
        wTrig.trackStop(noteoff + instOffset + layerMedio);
        wTrig.trackStop(noteoff + instOffset + layerForte);
        break;
      case 127:
        AfterPedal[KeyCount++] = noteoff + instOffset;
        if (KeyCount == (AfterPedalLength - 1)) KeyCount = 0;
        break;
    }
  }
}

void Sustain(byte ch, byte num, byte val) {
  if (num != 64) return;  // ignore non-sustain CC messages

  DamperState = val;

  if (val == 0) {  // pedal released: stop held notes
    for (int i = 0; i < AfterPedalLength; i++) {
      if (AfterPedal[i] == 0) continue;
      wTrig.trackStop(AfterPedal[i]);              // instOffset already baked in
      wTrig.trackStop(AfterPedal[i] + layerMedio);
      wTrig.trackStop(AfterPedal[i] + layerForte);
      AfterPedal[i] = 0;
    }
    KeyCount = 0;
  }
}

void loop() {

  MIDI.read();


  if ((digitalRead(BUTTON_PIN) == LOW) && (buttonClicked == false) && (millis() - lastDebounceTime > debounceDelay)) { //for Normally Open switches invert all (BUTTON_PIN) == LOW to (BUTTON_PIN) == HIGH and viceversa
    lastDebounceTime = millis();
    instSel = (instSel + 1) % NUM_LEDS;
    updateLEDs();
    buttonClicked = true;
  }

  if ((digitalRead(BUTTON_PIN) == HIGH) && (buttonClicked == true) && (millis() - lastDebounceTime > debounceDelay)) {
    lastDebounceTime = millis();
    buttonClicked = false;
  }

  switch (instSel) {
    case 0:
      instOffset = 0;
      break;
    case 1:
      instOffset = 264;
      break;
    case 2:
      instOffset = 528;
      break;
    case 3:
      instOffset = 792;
      break;
    case 4:
      instOffset = 1056;
      break;
    case 5:
      instOffset = 1320;
      break;
    default:
      instOffset = 0;
      break;
  }
}

void updateLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(ledPins[i], (i == instSel) ? HIGH : LOW);
  }
}
