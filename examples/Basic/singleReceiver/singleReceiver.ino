/*
 * Basic milesTag example, waits for incoming packets
 */

#include <milesTag.h>                     //Include the milesTag library

void setup() {
  Serial.begin(115200);                   //Set up Serial for debug output
  //milesTag.debug(Serial);                 //Send milesTag debug output to Serial (optional)
  milesTag.begin(milesTag.receiver);      //Simple single receiver requires basic initialisation
  milesTag.setReceivePin(34);             //Set the receive pin, which is mandatory
}

void loop() {
  if(milesTag.dataReceived())             //There is something in the packet buffer of the first 'busy' receiver. Multiple receivers can be busy and are handled individually.
  {
    Serial.print(F("Received "));
    if(milesTag.receivedDamage())
    {
      Serial.print(milesTag.receivedDamage());
      Serial.print(F(" damage from player ID:"));
      Serial.print(milesTag.receivedPlayerId());
      Serial.print(F(" team ID:"));
      Serial.println(milesTag.receivedTeamId());
    }
    else
    {
      Serial.println(F("message"));
    }
    milesTag.resumeReception();           //Clear the first 'busy' receiver buffer and prepare it for another packet. It is not automatically cleared
  }
}
