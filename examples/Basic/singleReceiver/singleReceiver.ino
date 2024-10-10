/*
 * Basic milesTag example, waits for incoming packets
 */

#include <milesTag.h>                     //Include the milesTag library

void setup() {
  Serial.begin(115200);                   //Set up Serial for debug output
  milesTag.debug(Serial);                 //Send milesTag debug output to Serial (optional)
  milesTag.begin(milesTag.receiver);      //Simple single receiver requires basic initialisation
  milesTag.setReceivePin(34);             //Set the receive pin, which is mandatory
}

void loop() {
  if(milesTag.dataReceived())
  {
    Serial.println(F("Data"));
  }
}
