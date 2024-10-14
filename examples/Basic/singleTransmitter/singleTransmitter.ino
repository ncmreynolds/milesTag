/*
 * Basic milesTag example, sends random damage every 5s from a single transmitter device
 */

#include <milesTag.h>                     //Include the milesTag library

void setup() {
  Serial.begin(115200);                   //Set up Serial for debug output
  //milesTag.debug(Serial);                 //Send milesTag debug output to Serial (optional)
  milesTag.begin();                       //Simple single transmitter requires no other initialisation
  milesTag.setTransmitPin(12);            //Set the transmit pin, which is mandatory
  milesTag.setPlayerId(random(0,128));    //Set random player ID 0-127
  milesTag.setTeamId(random(0,4));        //Set random team ID 0-3
}

void loop() {
  Serial.println(F("Firing random damage"));
  milesTag.transmitDamage(random(1,101)); //Transmit 1-100 damage from the first transmitter. Note milesTag only has 16 damage steps (1,2,4,5,7,10,15,17,20,25,30,35,40,50,75,100) so this will often be rounded down before sending
  delay(10e3);
}
