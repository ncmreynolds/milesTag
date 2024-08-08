/*
 * Basic milesTag example, sends 1 damage every 10s from a single transmitter device
 */

#include <milesTag.h>                     //Include the milesTag library

void setup() {
  Serial.begin(115200);                   //Set up Serial for debug output
  milesTag.debug(Serial);                 //Send milesTag debug output to Serial (optional)
  milesTag.begin();                       //Simple single transmitter requires no other initialisation
  milesTag.setTransmitPin(23);            //Set the transmit pin, which is mandatory
}

void loop() {
  milesTag.transmitDamage();              //Transmit one damage from the first transmitter
  delay(10e3);
}
