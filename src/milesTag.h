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
#include <Arduino.h>			//Standard Arduino library

#define SUPPORT_MILESTAG_TRANSMIT
#define SUPPORT_MILESTAG_RECEIVE

#if defined ESP32						//Use the RMT peripheral for ESP32
	#if defined SUPPORT_MILESTAG_TRANSMIT
		#define SUPPORT_RMT_TRANSMIT
		#include "driver/rmt_tx.h"
	#endif
	#if defined SUPPORT_MILESTAG_RECEIVE
		#define SUPPORT_RMT_RECEIVE
		#include "driver/rmt_rx.h"
	#endif
#endif

void recvIR(void* param);

class milesTagClass	{

	public:
		milesTagClass();														//Constructor function
		~milesTagClass();														//Destructor function
		enum class deviceType{transmitter, receiver, combo};					//Enum for device types
		static const deviceType transmitter = deviceType::transmitter;			//Convenience kludge for Arduino people
		static const deviceType receiver = deviceType::receiver;
		static const deviceType combo = deviceType::combo;
		#if defined SUPPORT_MILESTAG_TRANSMIT
			void setCarrierFrequency(uint16_t frequency);							//Must be done before begin(), default is 56000
			void setDutyCycle(uint8_t duty, uint8_t transmitterIndex = 0);			//Must be done before begin(), default is 50 and very unlikely to change
			bool setTransmitPin(int8_t pin);										//Set transmit pin for a single transmitter device
			bool setTransmitPins(int8_t* pins);										//Set transmit pins for a multi-transmitter device
			//Transmission
			bool transmitDamage(uint8_t damage = 1,									//Send damage on the specified transmitter, defaults to 1 damage on the first transmitter
				uint8_t transmitterIndex = 0,
				bool wait = false);
		#endif
		#if defined SUPPORT_MILESTAG_RECEIVE
			bool setReceivePin(int8_t pin, bool inverted = true);					//Set receive pin for a single transmitter device
			bool setReceivePins(int8_t* pins);										//Set receive pins for a multi-receiver device
			bool dataReceived();
			//Dumb semaphore
			volatile bool ir_data_received_ = 0;
			size_t len = 0;
		#endif
		bool begin(deviceType typeToIntialise = deviceType::transmitter,
			uint8_t numberOfTransmitters = 1,
			uint8_t numberOfReceivers = 1
			);																	//Start milesTag, pins are set with the functions below
		//Game information
		void setPlayerId(uint8_t id);											//Set the player ID, which can be 0-127, default 1
		void setTeamId(uint8_t id);												//Set the player team ID, which can be 0-3, default 0
		//Debug
		void debug(Stream &);													//Enable debugging on a stream, eg. Serial, which must already be started
		//Dumb semaphores
		volatile uint8_t irTX = 0;
		volatile uint8_t irRX = 0;
		//Debug
		Stream *debug_uart_ = nullptr;											//The stream used for debugging
	protected:
	private:
		//Game data
		uint8_t player_id_ = 1;													//Can be 0-127
		uint8_t team_id_ = 0;													//Can be 0-127
		uint8_t current_free_channel_ = 1;
		uint8_t current_free_memory_block_ = 1;
		static const uint16_t tx_start_on_time_ = 2400;							//Start on time  ie. how long to send carrier for to indicate a start bit
		static const uint16_t tx_zero_on_time_ = 600;							//Zero on time ie. how long to send carrier for to indicate a zero bit
		static const uint16_t tx_one_on_time_ = 1200;							//One on time ie. how long to send carrier for to indicate a one bit
		static const uint16_t tx_off_time_ = 600;								//Off time ie. how long to leave between bits
		deviceType type = deviceType::transmitter;								//Type of device, which alters behaviour/setup
		bool transmitters_configured_ = false;
		bool receivers_configured_ = false;
		#if defined SUPPORT_MILESTAG_TRANSMIT || defined SUPPORT_MILESTAG_RECEIVE
			uint8_t maximum_number_of_symbols_ = 192;								//Absolute maximum number of symbols
		#endif
		#if defined SUPPORT_MILESTAG_TRANSMIT
			//Global settings
			uint8_t number_of_transmitters_ = 0;									//Number of transmitter channels, usually 1-2
			#if defined SUPPORT_RMT_TRANSMIT
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
			/*
			typedef struct {
				rmt_encoder_t base;
				rmt_encoder_t *bytes_encoder;
				rmt_encoder_t *copy_encoder;
				int state;
				rmt_symbol_word_t reset_code;
			} milestag_encoder_t_;
			milestag_encoder_t_* encoder = nullptr;
			*/
			rmt_encoder_t *copy_encoder_;											//We will use a 'copy encoder' and do all encoding ourselves
			rmt_copy_encoder_config_t copy_encoder_config_ = {};					//The copy encoder supports no configuration, but must exist
			rmt_channel_handle_t* infrared_transmitter_handle_ = nullptr;			//RMT transmitter channels
			rmt_tx_channel_config_t* infrared_transmitter_config_ = nullptr;		//The RMT configuration for the transmitter(s)
			rmt_symbol_word_t** symbols_to_transmit_ = nullptr;						//Symbol buffers
			uint8_t* number_of_symbols_to_transmit_ = nullptr;
			#endif
			bool configure_tx_pin_(uint8_t index, int8_t pin);						//Configure a pin for TX on the current available channel
			//Damage
			void populate_buffer_with_damage_data_(uint8_t transmitterIndex,		//Build a simple 'damage' packet for transmission, this includes the preamble
				uint8_t damage);
			uint8_t map_damage_to_bitmask_(uint8_t damage);							//Turn a numeric damage value into a bitmask for packing into a packet
			//Transmission
			bool transmit_stored_buffer_(uint8_t transmitterIndex,					//Transmit a buffer from the specified transmitter channel
				rmt_symbol_word_t* buffer,
				uint8_t bufferLength,
				bool wait = false);
		#endif
		#if defined SUPPORT_MILESTAG_RECEIVE
			//Global settings
			uint8_t number_of_receivers_ = 0;										//Number of receiver channels, usually 1
			uint32_t hit_time_ = 1500;												//Time between hits
			uint32_t hit_timer_ = 0;												//Timer for hits
			#if defined SUPPORT_RMT_RECEIVE
			rmt_receive_config_t global_receiver_config_ = {						//Global config across all receivers
				.signal_range_min_ns = 2e3,											//Actually 600us but 2us is the smallest acceptable value in the SDK
				.signal_range_max_ns = 2800e3,										//Actually 2400us but allow some margin
			};
			//Receiver RMT data
			rmt_symbol_word_t** received_symbols_;									//Symbol buffers
			rmt_rx_channel_config_t* infrared_receiver_config_ = nullptr;			//The RMT configuration for the receiver(s)
			rmt_channel_handle_t* infrared_receiver_handle_ = nullptr;				//RMT receiver channels
			rmt_rx_done_event_data_t* rx_event_data_ = nullptr;						//RMT receiver event data
			bool rx_done_callback_(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data);
			#endif
			bool configure_rx_pin_(uint8_t index, int8_t pin, bool inverted = true);//Configure a pin for RX on the current available channel
			void resumeReception();													//Resume reception on all channels
			//Damage
			uint8_t map_bitmask_to_damage_(uint8_t bitmask);						//Turn a bitmask value into a numeric damage value when unpacking a packet
		#endif
		//Utilities
		//rmt_channel_t index_to_channel_(uint8_t index);							//Maps an integer index to an RMT channel
		//gpio_num_t int8_t_to_gpio_num_t(int8_t pin);							//Maps an integet pin to a gpio_num_t
};
extern milesTagClass milesTag;	//Create an instance of the class, as only one is practically usable at a time
#endif
