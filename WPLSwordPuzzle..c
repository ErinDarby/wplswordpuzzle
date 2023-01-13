/**
 * Slipring Sword, Copyright (c) 2022 Playful Technology
 * Draws various LED chase sequences on an LED strip (differing length, colour, speed)
 * and plays an MP3 sound effect triggered when different sensors are activated
 *
 * See https://youtu.be/zziWP7o8cHY for a video demonstration
 */

// INCLUDES
// Programmable LED library, see http://fastled.io
#include <FastLED.h>
// Software emulated serial interface
#include <AltSoftSerial.h>
// Serial MP3 player, see https://github.com/MajicDesigns/MD_YX5300
#include "src/MD_YX5300/MD_YX5300.h"

// DEFINES
// The total number of LEDs in the strip
#define NUM_LEDS 12

// CONSTANTS
const byte ledDataPin = A5;
const byte numSensors = 6;
const byte sensorPins[numSensors] = {2, 3, 4, 5, 6, 7};
// Define properties for the LEDs associated with each sensor
const byte hues[numSensors] = {0, 24, 50, 90, 120, 160};
const byte lengths[numSensors] = {2, 3, 4, 5, 6, 7};

// GLOBALS
// Define an array of RGB values
CRGB leds[NUM_LEDS];
// Initialise a software serial interface on the approriate Rx/Tx pins (8/9)
AltSoftSerial altSerial;
// And create an MP3 object based on the serial connection
MD_YX5300 mp3(altSerial);

// LED bar properties
int pos16 = 0; // Position of the LED bar in the chase sequence (1/16ths of a pixel)
int delta16 = 1; // How far to move the LED bar along the strip each frame (1/16ths of a pixel)
int hue = 20; // LED bar colour
int brightness = 0; // LED bar brightness
int length = 1; // Length of the LED bar (pixels)

// Was the sword pointing at one of the symbols in the last frame?
bool wasOverSensor = false;

void setup() {
  // Create a serial connection (only used for debugging)
  Serial.begin(115200);
  Serial.println(__FILE__ __DATE__);
 
  // Initialise the LEDs
  FastLED.addLeds<WS2812B, ledDataPin, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(128);
 
  // Initialise the input pins for the sensors
  for(int i=0; i<numSensors; i++){
    pinMode(sensorPins[i], INPUT_PULLUP);
  }
 
  // Initialise the serial MP3 player
  altSerial.begin(9600);
  mp3.begin();
  // Send messages using simple synchronous mode
  mp3.setSynchronous(true);
  // Volume is a value from 0-30
  mp3.volume(30);
}
 
// Draw a smooth "Light Bar" on the LED strip, starting at position 'pos16',
// expressed in 1/16ths of a pixel
void drawLightbar(int pos16, int length, int hue) {
  // Get the integer pixel number corresponding to this position
  int i = pos16 / 16;
  // Get the fractional part of the position
  int frac = pos16 & 0x0F;

  // The first pixel is at the end of the line, so its brightness is the "remainder"
  // of the fractional part, which is why we subtract from 255
  int firstpixelbrightness = 255 - (frac * 16);
 
  // The last pixel's brightness is the complement of the first pixel
  int lastpixelbrightness  = 255 - firstpixelbrightness;
 
  // For a bar of length "N", the code has to consider "N+1" pixel positions,
  // which is why the "<= length" below instead of "< length".
  int bright;
  // Loop over each pixel in the bar
  for(int n=0; n<=length; n++) {
    if(n==0) {
      // First pixel in the bar
      bright = firstpixelbrightness;
    } else if( n == length ) {
      // Last pixel in the bar
      bright = lastpixelbrightness;
    } else {
      // Middle pixels
      bright = 255;
    }
   
    leds[i] += CHSV(hue, 255, bright);
    i++;
    // Wrap around
    if(i == NUM_LEDS) i=0;
  }
}

void loop() {
  // Is the sword currently over one of the sensors?
  bool isOverSensor = false;
  for(int i=0; i<numSensors; i++) {
    // Set properties corresponding to the current sensor
    if(digitalRead(sensorPins[i]) == LOW){
      length = lengths[i];
      hue = hues[i];
      isOverSensor = true; // Update flag
    }
  }

  // If the sword is currently over a sensor...
  if(isOverSensor) {
    // ...and it wasn't before
    if(!wasOverSensor) {
      // Reset the position of the LED bar
      pos16 = 0;
      // Play the sound effect
      mp3.playTrack(1);
    }
    // Set full brightness
    brightness = 255;
  }
  // If the sword is not currently over a sensor...
  else {
    // ...let the LEDs fade
    brightness *= 0.8f;
    if(brightness<0) { brightness = 0; }
  }
  FastLED.setBrightness(brightness);
 
  // Update the flag
  wasOverSensor = isOverSensor;

  // Reset the previous array of LED RGB values
  memset8(leds, 0, NUM_LEDS * sizeof(CRGB));

  // Draw the light bar with the appropriate parameters
  drawLightbar(pos16, length, hue);
 
  // Advance the position of the bar
  pos16 += delta16;
 
  // Wrap around at end
  // Since pos16 contains position in "16ths of a pixel", the last position is (NUM_LEDS * 16)
  if( pos16 >= (NUM_LEDS * 16)) {
    pos16 -= (NUM_LEDS * 16);
  }

  // Send the updated RGB array to the LED strip
  FastLED.show();
  // 10ms pause before next frame
  delay(10);
}