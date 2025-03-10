/*
 *	An Arduino library for milesTag support
 *
 *	https://github.com/ncmreynolds/milesTag
 *
 *	Released under LGPL-2.1 see https://github.com/ncmreynolds/milesTag/LICENSE for full license
 *
 */
#ifndef milesTag_h
#define milesTag_h
#include <Arduino.h>						//Standard Arduino library

#define SUPPORT_MILESTAG_TRANSMIT
#define SUPPORT_MILESTAG_RECEIVE

#if defined ESP32
	#include <esp32rmtHelpers.h>				//Use the RMT peripheral for ESP32
#endif


void recvIR(void* param);

class milesTagClass	{

	public:
		milesTagClass();														//Constructor function
		~milesTagClass();														//Destructor function
		enum class deviceType{transmitter, receiver, combo};					//Enum for device types
		static const deviceType transmitter = deviceType::transmitter;			//Convenience kludge for Arduino ease of use
		static const deviceType receiver = deviceType::receiver;
		static const deviceType combo = deviceType::combo;
		#if defined SUPPORT_MILESTAG_TRANSMIT
			bool setTransmitPin(int8_t pin);									//Set transmit pin for a single transmitter device
			bool setTransmitPins(int8_t* pins);									//Set transmit pins for a multi-transmitter device
			//Transmission
			bool transmitDamage(uint8_t damage = 1,								//Send damage on the specified transmitter, defaults to 1 damage on the first transmitter
				uint8_t transmitterIndex = 0,
				bool wait = false);
		#endif
		#if defined SUPPORT_MILESTAG_RECEIVE
			bool setReceivePin(int8_t pin, bool inverted = true);				//Set receive pin for a single transmitter device
			bool setReceivePins(int8_t* pins);									//Set receive pins for a multi-receiver device
			bool dataReceived();												//Check if data has been received
			uint8_t receivedDamage();											//Amount of damage received, 0 implies a message rather than damage
			uint8_t receivedPlayerId();											//Received player ID in damage or message
			uint8_t receivedTeamId();											//Received team ID in damage or message
			bool resumeReception();												//Resume reception on the first 'busy' channel, false if no channel was busy
		#endif
		bool begin(deviceType typeToIntialise = deviceType::transmitter,
			uint8_t numberOfTransmitters = 1,
			uint8_t numberOfReceivers = 1
			);																	//Start milesTag, pins are set with the functions below
		//Game settings
		void setPlayerId(uint8_t id);											//Set the player ID, which can be 0-127, default 1
		void setTeamId(uint8_t id);												//Set the player team ID, which can be 0-3, default 0
		uint8_t playerId();														//Get the player ID, which can be 0-127
		uint8_t teamId();														//Get the player team ID, which can be 0-3
		//Debug
		void debug(Stream &);													//Enable debugging on a stream, eg. Serial, which must already be started
		//Debug
		Stream *debug_uart_ = nullptr;											//The stream used for debugging
	protected:
	private:
		deviceType type = deviceType::transmitter;								//Type of device, which alters behaviour/setup
		bool transmitters_configured_ = false;
		bool receivers_configured_ = false;
		//Game data
		uint8_t player_id_ = 1;													//Can be 0-127
		uint8_t team_id_ = 0;													//Can be 0-3
		#if defined SUPPORT_MILESTAG_TRANSMIT
			//Global settings
			esp32rmtTransmitHelper* transmitHelper = nullptr;						//Instance of transmit helper, only initialised if set up to transmit
			uint8_t maximum_number_of_transmit_symbols_ = 48;						//Absolute maximum number of symbols
			uint8_t number_of_transmitters_ = 0;									//Number of transmitter channels, usually 1-2
			uint8_t number_of_successfully_configured_transmitters_ = 0;			//How many transmitters were succesfully configured
			//Protocol timing
			static const uint16_t tx_start_on_time_ = 2400;							//Start on time  ie. how long to send carrier for to indicate a start bit
			static const uint16_t tx_zero_on_time_ = 600;							//Zero on time ie. how long to send carrier for to indicate a zero bit
			static const uint16_t tx_one_on_time_ = 1200;							//One on time ie. how long to send carrier for to indicate a one bit
			static const uint16_t tx_off_time_ = 600;								//Off time ie. how long to leave between bits
			//Encoding packets
			bool populate_buffer_with_damage_data_(uint8_t transmitterIndex,		//Build a simple 'damage' packet for transmission, this includes the preamble
				uint8_t damage);
			uint8_t map_damage_to_bitmask_(uint8_t damage);							//Turn a numeric damage value into a bitmask for packing into a packet
		#endif
		#if defined SUPPORT_MILESTAG_RECEIVE
			//Global settings
			esp32rmtReceiveHelper* receiveHelper = nullptr;							//Instance of receive helper, only initialised if set up to receive
			uint8_t maximum_number_of_receive_symbols_ = 64;						//Absolute maximum number of symbols
			uint8_t number_of_receivers_ = 0;										//Number of receiver channels, usually 1
			uint8_t number_of_successfully_configured_receivers_ = 0;				//How many receivers were succesfully configured
			//Parsed hit detail
			uint8_t received_player_id_ = 0;										//Can be 0-127
			uint8_t received_team_id_ = 0;											//Can be 0-3
			uint8_t received_damage_ = 0;											//Can be 1-100 but is derived from a bitmask
			//Buffer for incoming message
			static const uint8_t maximum_message_length_ = 3;						//Maximum size of a milesTag message
			uint8_t** message_data_ = nullptr;										//One set of message data per receiver
			//Protocol timing
			static const uint16_t start_bit_low_watermark_ = 2200;
			static const uint16_t start_bit_high_watermark_ = 2480;
			static const uint16_t zero_bit_low_watermark_ = 590;
			static const uint16_t zero_bit_high_watermark_ = 680;
			static const uint16_t one_bit_low_watermark_ = 1190;
			static const uint16_t one_bit_high_watermark_ = 1280;
			static const uint16_t gap_low_watermark_ = 520;
			static const uint16_t gap_high_watermark_ = 680;
			//Parsing packets
			bool parse_received_symbols_(uint8_t index);							//Parse a buffer of pulse timings
			uint8_t characterise_symbol_(uint8_t index,uint8_t  symbol_index_);		//Parse an individual symbol
			uint8_t map_bitmask_to_damage_(uint8_t bitmask);						//Turn a bitmask value into a numeric damage value when unpacking a packet
		#endif
};
extern milesTagClass milesTag;	//Create an instance of the class, as only one is practically usable at a time
#endif
