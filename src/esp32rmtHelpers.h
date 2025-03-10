/*
 *	An Arduino library to provide 'helpers' for the RMT peripheral on ESP32 when sending remote-control style infrared signals
 *
 *	Mostly it is used in the following libraries that are for making Laser-Tag equipment
 *
 *	https://github.com/ncmreynolds/milesTag
 *	https://github.com/ncmreynolds/WoW
 *	https://github.com/ncmreynolds/DoT (closed source)
 *
 *	Released under LGPL-2.1 see https://github.com/ncmreynolds/esp32rmtHelpers/LICENSE for full license
 *
 */
#ifndef esp32rmtHelpers_h
#include <Arduino.h>						//Standard Arduino library


#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"

class esp32rmtTransmitHelper	{
	public:
		esp32rmtTransmitHelper();												//Constructor function
		~esp32rmtTransmitHelper();												//Destructor function
		bool begin(uint8_t numberOfTransmitters = 1);							//Initialise one or more transmitters
		void setCarrierFrequency(uint16_t frequency);							//Must be done before begin(), default is 56000
		void setDutyCycle(uint8_t duty, uint8_t transmitterIndex = 0);			//Must be done before begin(), default is 50 and very unlikely to change
		void setMaximumNumberOfSymbols(uint8_t symbols);						//Must be done before begin(), default is 64
		bool configure_tx_pin_(uint8_t index, int8_t pin);						//Configure a pin for TX on the current available channel
		bool transmitterBusy(uint8_t index);									//Can check if busy before starting another transmission
		uint8_t maximumNumberOfSymbols();										//Maximum number of symbols
		bool addSymbol(uint8_t index, uint16_t duration0, uint8_t level0,		//Add a symbol to the buffer for the specified transmitter channel
			uint16_t duration1, uint8_t level1);
		bool transmit_stored_buffer_(uint8_t transmitterIndex,					//Transmit a buffer from the specified transmitter channel
				bool wait = false);

		//Debug
		void debug(Stream &);													//Enable debugging on a stream, eg. Serial, which must already be started
		Stream *debug_uart_ = nullptr;											//The stream used for debugging

	protected:

	private:
		//Global settings
		uint8_t maximum_number_of_symbols_ = 48;								//Absolute maximum number of symbols, minimum is 48
		uint8_t number_of_transmitters_ = 0;									//Number of transmitter channels, usually 1-2
		rmt_carrier_config_t global_transmitter_config_ = {						//Global config across all receivers
			.frequency_hz = 56000,
			.duty_cycle = 0.50,
			//.flags = {
			//	.polarity_active_low = 0
			//}
		};
		rmt_transmit_config_t event_transmitter_config_ = {
			//.eot_level = 0,														//Drive pin low at end
			//.loop_count = 0,													//Do not loop
		};
		rmt_encoder_t *copy_encoder_;											//We will use a 'copy encoder' and do all encoding ourselves
		rmt_copy_encoder_config_t copy_encoder_config_ = {};					//The copy encoder supports no configuration, but must exist
		rmt_channel_handle_t* infrared_transmitter_handle_ = nullptr;			//RMT transmitter channels
		rmt_tx_channel_config_t* infrared_transmitter_config_ = nullptr;		//The RMT configuration for the transmitter(s)
		rmt_symbol_word_t** symbols_to_transmit_ = nullptr;						//Symbol buffers
		uint8_t* number_of_symbols_to_transmit_ = nullptr;						//Count of symbols to avoid going past end of buffer

};

class esp32rmtReceiveHelper	{
	public:
		esp32rmtReceiveHelper();														//Constructor function
		~esp32rmtReceiveHelper();														//Destructor function
		bool begin(uint8_t numberOfReceivers = 1);										//Initialise one or more receivers

		//Debug
		Stream *debug_uart_ = nullptr;													//The stream used for debugging

	protected:

	private:
		uint8_t maximum_number_of_symbols_ = 64;								//Absolute maximum number of symbols
		static const uint8_t maximum_message_length_ = 3;						//Maximum size of a milesTag message
		uint8_t number_of_receivers_ = 0;										//Number of receiver channels, usually 1
		rmt_receive_config_t global_receiver_config_ = {						//Global config across all receivers
			.signal_range_min_ns = 2e3,											//Actually 600us but 2us is the smallest acceptable value in the SDK
			.signal_range_max_ns = 2800e3,										//Actually 2400us but allow some margin
		};
		//Receiver RMT data
		rmt_symbol_word_t** received_symbols_;									//Symbol buffers
		uint8_t* number_of_received_symbols_ = nullptr;							//Count of symbols in the buffer
		rmt_rx_channel_config_t* infrared_receiver_config_ = nullptr;			//The RMT configuration for the receiver(s)
		rmt_channel_handle_t* infrared_receiver_handle_ = nullptr;				//RMT receiver channels
		uint8_t** message_data_ = nullptr;										//One message data buffer per receiver
};

#endif