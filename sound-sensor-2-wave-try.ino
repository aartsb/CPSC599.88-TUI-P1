/*
  Example of oversampling analog input from a thermistor
  for increased resolution.  It's a basic attempt at a biofeedback
  device used as an ineffective treatment for migraines.  The idea
  is that if you can focus on making your hands warm, increased blood
  flow to the extremities is associated with a reduced stress response.
  Anyway, the bleeps sweep up if the temperature increases, down for decrease,
  and level for no change.  The tremelo rate increases with the temperature.
  Using Mozzi sonification library.

  Demonstrates OverSample object.

  The circuit:
     Audio output on digital pin 9 on a Uno or similar, or
    DAC/A14 on Teensy 3.1, or
     check the README or http://sensorium.github.io/Mozzi/

  Temperature dependent resistor (Thermistor) and 5.1k resistor on analog pin 1:
    Thermistor from analog pin to +5V (3.3V on Teensy 3.1)
    5.1k resistor from analog pin to ground

  Mozzi documentation/API
  https://sensorium.github.io/Mozzi/doc/html/index.html

  Mozzi help/discussion/announcements:
  https://groups.google.com/forum/#!forum/mozzi-users

  Tim Barrass 2013, CC by-nc-sa.
 */

#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <Line.h>
#include <tables/sin2048_int8.h> // SINe table for oscillator
#include <OverSample.h>
#include <ControlDelay.h>

// use: Oscil <table_size, update_rate> oscilName (wavetable)
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aTremelo(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aEnvelope(SIN2048_DATA);

Line <float> freqLine;

OverSample <unsigned int, 3> overSampler; // will give 10+3=13 bits resolution, 0->8191, using 128 bytes

int INPUT_PIN = A0;
const long SAMPLE_TIME = 2;
unsigned long millisCurrent;
unsigned long millisLast = 0;
unsigned long millisElapsed = 0;

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 128 // Hz, powers of 2 are most reliable

float ENVELOPE_DURATION = 1.f;

const byte LINE_LENGTH = (byte)((float)CONTROL_RATE*ENVELOPE_DURATION*0.5); // 0.5 seconds per line

// adjustments to get tremelo in useful range from oversampled blow input
const int TREMOLO_OFFSET = 2000;
const float TREMOLO_SCALE = 0.002;

void setup(){
  //pinMode(INPUT_PIN,INPUT);
  
  Serial.begin(115200);
  aEnvelope.setFreq(ENVELOPE_DURATION);
  startMozzi(CONTROL_RATE);
}


void updateControl(){
  float start_freq, end_freq;
  static int counter, old_oversampled;

  // read the variable resistor
  int sensor_value = mozziAnalogRead(INPUT_PIN); // value is 0-1023

  //achieving accuracy 
  millisCurrent = millis();
  millisElapsed = millisCurrent - millisLast;

  if (millisElapsed >= SAMPLE_TIME) {
    millisLast = millisCurrent;
    Serial.println(sensor_value);
    //Serial.println("a");
    //sampleBufferValue = 0;
 
  // get the next oversampled sensor value
  int oversampled = overSampler.next(sensor_value);
  
  //Modulation:
  // modulate the amplitude of the sound in proportion to the magnitude of the oversampled sensor
  float tremeloRate = TREMOLO_SCALE*(oversampled-TREMOLO_OFFSET);
  tremeloRate = tremeloRate*tremeloRate*tremeloRate*tremeloRate*tremeloRate;
  aTremelo.setFreq(tremeloRate);

  // every half second
   if (--counter<0){

    if (oversampled>old_oversampled){ 
      start_freq = 144.f;
      end_freq = 150.f;
    }else if(oversampled<old_oversampled){ 
      start_freq = 150.f;
      end_freq = 160.f;
    } else { 
      start_freq = 160.f;
      end_freq = 174.f;
    }
    old_oversampled = oversampled;
    counter = LINE_LENGTH-1; // reset counter

    // set the line to change the main frequency
    freqLine.set(start_freq,end_freq,LINE_LENGTH);

    // print out for debugging
    //Serial.print(sensor_value);Serial.print("\t");
    Serial.print(oversampled);Serial.print("\t");Serial.print(start_freq);Serial.print("\t");Serial.println(end_freq);
  }
  }

  // update the main frequency of the sound
  aSin.setFreq(freqLine.next());

}


int updateAudio(){
  return ((((int)aSin.next()*(128+aTremelo.next()))>>8)*aEnvelope.next())>>8;
}


void loop(){
  audioHook(); // required here
}
