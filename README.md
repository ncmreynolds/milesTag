# MilesTag
This is an Arduino library for building MilesTag 2 protocol based Lasertag hardware using the ESP32 RMT peripheral. It is not a full instructional repository for making fully functional Lasertag equipment, just a library to build other things with. The example sketches should be a good start for you.

While many, much simpler microcontrollers are absolutely capable of building Lasertag weapons the ESP32 was chosen as the target for two reasons.

1. The ESP32 has the RMT hardware peripheral specifically for handling arbitrary timing critical transactions without involving the CPU
2. The ESP32 is an excellent choice for all the other things you are likely to want from a functional Lasertag weapon...
   - Generating sound with I2S
   - Driving a display
   - Configuring it over Bluetooth
   - Sending game stats over WiFi to a game server

In many ways this is a case of "the tail wagging the dog" but for low volume hobby level use ESP32 modules are not consequentially more expensive than other options. The ESP32C3 is an excellent low cost option for this use case and if you lower the CPU speed and disable WiFi/BLE when it's not needed then the power usage drops significantly.

## To-Do

- More fully featured examples that work as usable weapons and sensors
- Implement 'message' packets for healing, powerups and so on

## Release History

- 0.1.1 - Functional send, receive and combo use for 'shot' packets, migrated to ESP-IDF v5.x / Arduino core v3.x (not backwards compatible)
- 0.1.0 - Initial release, transmit only and specific to ESP-IDF v4.x / Arduino core v2.x
