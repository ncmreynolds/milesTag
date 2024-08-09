/*
 * Twin transmitter milesTag example
 * 
 * It sends damage every 5s from a two different transmitter devices
 * 
 * You probably only need this if the devices have different physical characteristics like lenses or actual output power
 * 
 * A use case is a 'full power' emitter for use outdoors and a 'low power' emitter for use indoors, this could be the same LED but different power limiting resistors
 * 
 */

#include <milesTag.h>                         //Include the milesTag library

int8_t emitterPins[2] = {12, 13};             //GPIO pins are in an array

void setup() {
  Serial.begin(115200);                       //Set up Serial for debug output
  milesTag.debug(Serial);                     //Send milesTag debug output to Serial (optional)
  milesTag.begin(milesTag.transmitter,2);     //Twin transmitter requires fuller initialisation
  milesTag.setTransmitPins(emitterPins);      //Set the transmit pins, which are mandatory
}

void loop() {
  milesTag.transmitDamage();                  //Transmit one damage from the first transmitter, which is the default
  delay(10e3);
  milesTag.transmitDamage(2, 1);              //Transmit two damage from the second transmitter. Transmitter index starts at zero
  delay(10e3);
}
