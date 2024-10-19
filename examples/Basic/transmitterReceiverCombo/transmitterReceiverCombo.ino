/*
 * Combo milesTag example, sends random damage every 5s from a single transmitter device while also accepting hits
 * note a weapon will often 'shoot itself' due to reflected IR so this sketch ignores 'self' hits by comparing player and team IDs
 * 
 * You can choose to ignore these reflected hits or not in your own code
 */

#include <milesTag.h>                     //Include the milesTag library
uint32_t lastTransmit = 0;

void setup() {
  Serial.begin(115200);                   //Set up Serial for debug output
  //milesTag.debug(Serial);                 //Send milesTag debug output to Serial (optional)
  milesTag.begin(milesTag.combo);         //Initialise as a 'combo' device
  milesTag.setTransmitPin(12);            //Set the transmit pin, which is mandatory
  milesTag.setReceivePin(34);             //Set the receive pin, which is mandatory
  milesTag.setPlayerId(random(0,128));    //Set random player ID 0-127
  milesTag.setTeamId(random(0,4));        //Set random team ID 0-3
}

void loop() {
  if(milesTag.dataReceived())             //There is something in the packet buffer of the first 'busy' receiver. Multiple receivers can be busy and are handled individually.
  {
    Serial.print(F("Received "));
    if(milesTag.receivedDamage())
    {
      if(milesTag.receivedPlayerId() == milesTag.playerId() && milesTag.teamId() == milesTag.receivedTeamId())
      {
        Serial.println(F("reflected damage, ignoring"));
      }
      else
      {
        Serial.print(milesTag.receivedDamage());
        Serial.print(F(" damage from player ID:"));
        Serial.print(milesTag.receivedPlayerId());
        Serial.print(F(" team ID:"));
        Serial.println(milesTag.receivedTeamId());
      }
    }
    else
    {
      Serial.println(F("message"));
    }
    milesTag.resumeReception();           //Clear the first 'busy' receiver buffer and prepare it for another packet. It is not automatically cleared
  }
  else if(millis() - lastTransmit > 10e3)
  {
    lastTransmit = millis();
    Serial.println(F("Firing random damage"));
    milesTag.transmitDamage(random(1,101)); //Transmit 1-100 damage from the first transmitter. Note milesTag only has 16 damage steps (1,2,4,5,7,10,15,17,20,25,30,35,40,50,75,100) so this will often be rounded down before sending
  }
}
