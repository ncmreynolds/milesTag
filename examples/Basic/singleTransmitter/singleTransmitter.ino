/*
 * Basic milesTag example, sends random damage every 5s from a single transmitter device
 */

#include <milesTag.h>                     //Include the milesTag library

uint8_t damageSteps[16] = {1,2,4,5,7,10,15,17,20,25,30,35,40,50,75,100};
uint8_t damage = 1;

void setup() {
  Serial.begin(115200);                   //Set up Serial for debug output
  //milesTag.debug(Serial);                 //Send milesTag debug output to Serial (optional)
  milesTag.begin();                       //Simple single transmitter requires no other initialisation
  milesTag.setTransmitPin(10);            //Set the transmit pin, which is mandatory
}

void loop() {
  delay(10e3);
  milesTag.setPlayerId(random(0,128));    //Set random player ID 0-127
  milesTag.setTeamId(random(0,4));        //Set random team ID 0-3
  damage = damageSteps[random(0,16)];
  Serial.print(F("Transmitting "));
  Serial.print(damage);
  Serial.print(F(" damage from player ID:"));
  Serial.print(milesTag.playerId());
  Serial.print(F(" team ID:"));
  Serial.println(milesTag.teamId());
  milesTag.transmitDamage(damage); //Transmit 1-100 damage from the first transmitter.
}
