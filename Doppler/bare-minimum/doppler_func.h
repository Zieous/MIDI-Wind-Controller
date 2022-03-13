//see https://github.com/dadamachines/doppler for doppler info

#include "MIDIUSB.h"


void setupBoard(void){
  //Pin setup
  //Digital Pins 0-9 are key inputs
  //A3 is breath sensor
  //A5 is Octave buttons [0.33, 1.65, 2.97]V
  //A7 is MIDI out

  //Button setup
  for (int n=0;n<10;n++){
    pinMode(n,INPUT);
  }
}



void setupBreath(void){
  breath_at_rest = analogRead(BREATH_PIN);
  int n = 0, num_run = 100;
  for(n=0;n<num_run;n++){
    breath_at_rest = (breath_at_rest+float(analogRead(BREATH_PIN)))/2;
  }
  breath_at_rest = breath_at_rest/1024; //Convert to range 0-1
}


int readBreath(void){
  //breath_array
  float breath = 0;
  int n;
  int breath_sum = 0, breath_filt=0;//, breath_raw;
  //Read the breath sensor value
  breath = (float(analogRead(BREATH_PIN))/1024-breath_at_rest); //Remove atmospheric pressure
  //Scale the breath input
  if(breath<0){
    breath = -1*pow(-1*breath,breath_scale);
  }else{
    breath = pow(breath,breath_scale);
  }
  breath_raw = min(127,max(0,int(breath*127)));

  //breath_array code
  // Using pointers because I couldn't get normal array indexing to work

  // Shift values right through the breath array
  for (n=0;n<breath_array_len-1;n++){
    *(breath_array+n) = *(breath_array+(n+1));
  }
  //Add new breath value
  *(breath_array+breath_array_len-1) = breath_raw;
  //Average the breath value array
  for (n=0;n<breath_array_len;n++){
    breath_sum = breath_sum + *(breath_array+n);
  }
  breath_filt = breath_sum/breath_array_len;

  
  return breath_filt;
}


int readBreath_new(void){
  //breath_array
  float breath = 0;
  int n;
  int breath_sum = 0, breath_filt=0;//, breath_raw;
  //Read the breath sensor value
  breath = (float(analogRead(BREATH_PIN))/1024-breath_at_rest); //Remove atmospheric pressure
  //Scale the breath input
  if(breath<0){
    breath = -1*pow(-1*breath,breath_scale);
    breath_raw = min(0,max(-127,int(breath*127)));
  }else{
    breath = pow(breath,breath_scale);
    breath_raw = min(127,max(0,int(breath*127)));
  }
//  breath_raw = min(127,max(0,int(breath*127)));

  //breath_array code
  // Using pointers because I couldn't get normal array indexing to work

  // Shift values right through the breath array
  for (n=0;n<breath_array_len-1;n++){
    *(breath_array+n) = *(breath_array+(n+1));
  }
  //Add new breath value
  *(breath_array+breath_array_len-1) = breath_raw;
  //Average the breath value array
  for (n=0;n<breath_array_len;n++){
    breath_sum = breath_sum + *(breath_array+n);
  }
  breath_filt = breath_sum/breath_array_len;

  
  return breath_filt;
}

uint16_t readButtons(void){
  //Input - Button
//  0 - B
//  1 - A (C)
//  2 - G
//  3 - G sharp
//  4 - F
//  5 - E
//  6 - D
//  7 - D#
//  8 - Bb
//  9 - C low
//  A5 - Octave [0.33, 1.65, 2.97]V

byte n = 0;
uint16_t note_buttons = 0, note_midi = 0;
for(n=0;n<10;n++){
  note_buttons = note_buttons | (digitalRead(n)<<n);
}
switch (note_buttons){
    case 0:
    //C#
      note_midi = 73;
      break;
    case 2:
    //C
      note_midi = 72;
      break;
    case 257:
    //C alternate
      note_midi = 72;
      break;
    case 1:
    //B
      note_midi = 71;
      break;
    case 19:
    //A#
      note_midi = 70;
      break;
    case 259:
    //A# alternate
      note_midi = 70;
      break;
    case 3:
    //A
      note_midi = 69;
      break;
    case 15:
    //G#
      note_midi = 68;
      break;
    case 7:
    //G
      note_midi = 67;
      break;
    case 39:
    //F#
      note_midi = 66;
      break;
    case 23:
    //F
      note_midi = 65;
      break;
    case 55:
    //E
      note_midi = 64;
      break;
    case 247:
    //D#
      note_midi = 63;
      break;
    case 119:
    //D
      note_midi = 62;
      break;
    case 631:
    //C Low
      note_midi = 60;
      break;
    default:
      note_midi = -1;
  }

// Adjust the octave
  int octave_read = 0;
  octave_read = analogRead(A5);
  if (octave_read>720){
    //top octave
    note_midi += 24;
//    analogWrite(Octave_LED_Pin, 255);
  }else if(octave_read>302){
    //middle octave
    note_midi += 12;
//    analogWrite(Octave_LED_Pin, 64);
  }else if(octave_read>46){
    //bottom octave
    note_midi -= 12;
//    analogWrite(Octave_LED_Pin, 64);
  }else{
    //home octave
    note_midi += 0;
//    analogWrite(Octave_LED_Pin, 0);
  }

return note_midi;

}


// First parameter is the event type (0x09 = note on, 0x08 = note off).
// Second parameter is note-on/note-off, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the note number (48 = middle C).
// Fourth parameter is the velocity (64 = normal, 127 = fastest).

void USBnoteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush(); //This forces the note to be sent
}

void USBnoteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush(); //This forces the note to be sent
}

// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).

void USBcontrolChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush(); //This forces the note to be sent
}

void USB_PANIC(void) {
  int n = 0,k = 0;
  for(k=0;k<16;k++){
    for(n=0;n<128;n++){
      midiEventPacket_t noteOff = {0x08, 0x80 | k, n, 0};
      MidiUSB.sendMIDI(noteOff);
      MidiUSB.flush(); //This forces the note to be sent
    }
  }
}
