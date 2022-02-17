/*
  Using Mozzi library, making use of the OverSample, Control Delay classes to modulate the sound output 
  created by blowing into the sound sensor module fitted in the OceanBed Swerve instrument, giving the illusion of
  the performer being able to control anthropogenic sounds underwater. 
 */

#include <MozziGuts.h>
#include <Oscil.h> 
#include <Line.h>
#include <tables/sin2048_int8.h> // SIN table for oscillator
#include <OverSample.h>
#include <ControlDelay.h>

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

const int TREMOLO_OFFSET = 2000;
const float TREMOLO_SCALE = 0.002;

void setup(){
  
  Serial.begin(115200);
  aEnvelope.setFreq(ENVELOPE_DURATION);
  startMozzi(CONTROL_RATE);
}


void updateControl(){
  float start_freq, end_freq;
  static int counter, old_oversampled;

  // read the variable resistor
  int sensor_value = mozziAnalogRead(INPUT_PIN); // value is 0-1023

  //achieving accuracy by sampling 
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

    freqLine.set(start_freq,end_freq,LINE_LENGTH);

    // debugging
    //Serial.print(sensor_value);Serial.print("\t");
    Serial.print(oversampled);Serial.print("\t");Serial.print(start_freq);Serial.print("\t");Serial.println(end_freq);
  }
  }

  // update the main frequency of the sound
  aSin.setFreq(freqLine.next());

}

//formula for updated modulation
int updateAudio(){
  return ((((int)aSin.next()*(128+aTremelo.next()))>>8)*aEnvelope.next())>>8;
}


void loop(){
  audioHook(); 
}
