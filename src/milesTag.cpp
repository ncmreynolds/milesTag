/*
 *	An Arduino library for milesTag support
 *
 *	https://github.com/ncmreynolds/milesTag
 *
 *	Released under LGPL-2.1 see https://github.com/ncmreynolds/milesTag/LICENSE for full license
 *
 */
#ifndef milesTag_cpp
#define milesTag_cpp
#include "milesTag.h"


milesTagClass::milesTagClass()	//Constructor function
{
}

milesTagClass::~milesTagClass()	//Destructor function
{
}
bool milesTagClass::begin(deviceType typeToIntialise, uint8_t numberOfTransmitters, uint8_t numberOfReceivers) 
{
	type = typeToIntialise;
	#if defined SUPPORT_MILESTAG_TRANSMIT
		number_of_transmitters_ = numberOfTransmitters;
	#endif
	#if defined SUPPORT_MILESTAG_RECEIVE
		number_of_receivers_ = numberOfReceivers;
	#endif
	bool initialisation_success_ = true;
	#if defined SUPPORT_MILESTAG_TRANSMIT && defined SUPPORT_MILESTAG_RECEIVE
		if(type == deviceType::combo)
		{
			#if defined SUPPORT_RMT_TRANSMIT && ESP_IDF_VERSION_MAJOR < 5
				if(number_of_transmitters_ + numberOfReceivers + 1 > RMT_CHANNEL_MAX)
				{
					initialisation_success_ = false;
				}
			#endif
			if(debug_uart_ != nullptr)
			{
				debug_uart_->print(F("milesTag: creating combo device\r\n"));
			}
		}
	#endif
	#if defined SUPPORT_MILESTAG_TRANSMIT
		if(type == deviceType::transmitter)
		{
			#if defined SUPPORT_RMT_TRANSMIT && ESP_IDF_VERSION_MAJOR < 5
				if(number_of_transmitters_ + 1 > RMT_CHANNEL_MAX)
				{
					initialisation_success_ = false;
				}
			#endif
			if(debug_uart_ != nullptr)
			{
				debug_uart_->print(F("milesTag: creating transmitter device\r\n"));
			}
		}
	#endif
	#if defined SUPPORT_MILESTAG_RECEIVE
		if(type == deviceType::receiver)
		{
			#if defined SUPPORT_RMT_RECEIVE && ESP_IDF_VERSION_MAJOR < 5
				if(number_of_receivers_ + 1 > RMT_CHANNEL_MAX)
				{
					initialisation_success_ = false;
				}
			#endif
			if(debug_uart_ != nullptr)
			{
				debug_uart_->printf_P(PSTR("milesTag: creating %u receiver device(s)\r\n"), number_of_receivers_);
			}
		}
	#endif
	if(initialisation_success_ == false)
	{
		if(debug_uart_ != nullptr)
		{
			debug_uart_->println(F("milesTag: too many channels requested"));
		}
		return initialisation_success_;
	}
	//Possible workaround for RMT initialisation issue following an unexpected reset, eg. during programming
	//periph_module_disable(PERIPH_RMT_MODULE);
    //periph_module_enable(PERIPH_RMT_MODULE);
	#if defined SUPPORT_MILESTAG_TRANSMIT
		if(type == deviceType::transmitter || type == deviceType::combo)
		{
			#if defined SUPPORT_RMT_TRANSMIT
				infrared_transmitter_handle_ = new rmt_channel_handle_t[number_of_transmitters_];
				infrared_transmitter_config_ = new rmt_tx_channel_config_t[number_of_transmitters_];
				symbols_to_transmit_ = new rmt_symbol_word_t*[number_of_transmitters_];
				number_of_symbols_to_transmit_ = new uint8_t[number_of_transmitters_];
				//encoder = static_cast<milesTagClass::milestag_encoder_t_*>(rmt_alloc_encoder_mem(sizeof(milestag_encoder_t_)));		//Allocate the encoder
				if(rmt_new_copy_encoder(&copy_encoder_config_, &copy_encoder_) != ESP_OK)	//Initialise the copy encoder
				{
					if(debug_uart_ != nullptr)
					{
						debug_uart_->println(F("milesTag: failed to create copy encoder"));
					}
					initialisation_success_ = false;
				}
				else
				{
					if(debug_uart_ != nullptr)
					{
						debug_uart_->println(F("milesTag: created copy encoder"));
					}
				}
				for(uint8_t index = 0; index < number_of_transmitters_; index++)
				{
					symbols_to_transmit_[index] = new rmt_symbol_word_t[maximum_number_of_symbols_];
					number_of_symbols_to_transmit_[index] = 0;
				}
			#else
			#endif
			if(debug_uart_ != nullptr)
			{
				debug_uart_->printf_P(PSTR("milesTag: created %u TX channel(s)\r\n"), number_of_transmitters_);
			}
		}
	#endif
	#if defined SUPPORT_MILESTAG_RECEIVE
		if(type == deviceType::receiver || type == deviceType::combo)
		{
			#if defined SUPPORT_RMT_RECEIVE
				//Create RMT data structures for the receive channels (usually just one, but the intention is to support multiples)
				infrared_receiver_config_ = new rmt_rx_channel_config_t[number_of_receivers_];	//Create data structures
				infrared_receiver_handle_ = new rmt_channel_handle_t[number_of_receivers_];
				number_of_received_symbols_ = new uint8_t[number_of_receivers_];
				received_symbols_ = new rmt_symbol_word_t*[number_of_receivers_];
				for(uint8_t index = 0; index < number_of_receivers_; index++)
				{
					received_symbols_[index] = new rmt_symbol_word_t[maximum_number_of_symbols_];
					number_of_received_symbols_[index] = 0;
				}
			#else
			#endif
			if(debug_uart_ != nullptr)
			{
				debug_uart_->printf_P(PSTR("milesTag: created %u RX channel(s)\r\n"), number_of_receivers_);
			}
		}
	#endif
	return initialisation_success_;
}
#if defined SUPPORT_MILESTAG_TRANSMIT
	void milesTagClass::setCarrierFrequency(uint16_t frequency)	//Must be done before begin(), default is 56000
	{
		#if defined SUPPORT_RMT_TRANSMIT
			global_transmitter_config_.frequency_hz = frequency;
		#endif
	}
	void milesTagClass::setDutyCycle(uint8_t duty, uint8_t transmitterIndex)	//Must be done before begin(), default is 50 and very unlikely to change
	{
		#if defined SUPPORT_RMT_TRANSMIT
			global_transmitter_config_.duty_cycle = float(duty)/100.0;
		#endif
	}
	bool milesTagClass::setTransmitPin(int8_t pin)	//Set transmit pin for a single transmitter device
	{
		if((type == deviceType::transmitter || type == deviceType::combo) && infrared_transmitter_handle_ != nullptr)
		{
			transmitters_configured_ = configure_tx_pin_(0, pin);
			return transmitters_configured_;
		}
		return false;
	}
	bool milesTagClass::setTransmitPins(int8_t* pins)	//Set transmit pins for a multi-transmitter device
	{
		if((type == deviceType::transmitter || type == deviceType::combo) && infrared_transmitter_handle_ != nullptr)
		{
			uint8_t successfullyConfigured = 0;
			for(uint8_t index = 0; index < number_of_transmitters_; index++)
			{
				successfullyConfigured += (configure_tx_pin_(index, pins[index]) == true);
			}
			transmitters_configured_ = (successfullyConfigured == number_of_transmitters_);
			return transmitters_configured_;
		}
		return false;
	}
	bool rmt_tx_done_callback(rmt_channel_handle_t channel, const rmt_tx_done_event_data_t *edata, void *user_data)
	{
		*(uint8_t *)(user_data) = 0;	//Reset the symbol count, which shows this channel as free
		return false;
	}
	bool milesTagClass::configure_tx_pin_(uint8_t index, int8_t pin)
	{
		infrared_transmitter_config_[index] = {
			.gpio_num = static_cast<gpio_num_t>(pin),
			.clk_src = RMT_CLK_SRC_DEFAULT,
			.resolution_hz = 1000000, // 1MHz resolution, 1 tick = 1us
			.mem_block_symbols = maximum_number_of_symbols_,
			.trans_queue_depth = 4,
		};
		infrared_transmitter_config_[index].flags = {
			.with_dma = false,
			//.allow_pd = 1,
		};
		if(rmt_new_tx_channel(&infrared_transmitter_config_[index], &infrared_transmitter_handle_[index]) == ESP_OK)
		{
			rmt_tx_event_callbacks_t transmit_callbacks_ = {
                .on_trans_done = rmt_tx_done_callback
				//.on_recv_done = tx_done_callback_
            };
			rmt_tx_register_event_callbacks(infrared_transmitter_handle_[index], &transmit_callbacks_, &number_of_symbols_to_transmit_[index]);
			rmt_apply_carrier(infrared_transmitter_handle_[index], &global_transmitter_config_);
			rmt_enable(infrared_transmitter_handle_[index]);
			if(debug_uart_ != nullptr)
			{
				debug_uart_->printf_P(PSTR("milesTag: configured pin %u as transmitter %u at %.2fKHz %u%% duty cycle\r\n"), pin, index, float(global_transmitter_config_.frequency_hz/1000), uint8_t(global_transmitter_config_.duty_cycle*100));
			}
			return true;
		}
		else
		{
			if(debug_uart_ != nullptr)
			{
				debug_uart_->printf_P(PSTR("milesTag: failed to configure pin %u for TX\r\n"), pin);
			}
		}
		return false;
	}
	void milesTagClass::populate_buffer_with_damage_data_(uint8_t index, uint8_t damage)
	{
		uint8_t dataToSend[4] = {0,0};
		dataToSend[0] = player_id_ & B01111111;																		//Player ID is in bottom 7 bits of byte 0, the top bit is always zero for damage
		dataToSend[1] = ((uint8_t)team_id_) << 6;																	//Team ID is in top two bits of byte 1
		dataToSend[1] = dataToSend[1] | ((map_damage_to_bitmask_(damage)) << 2);									//Damage is in next four bits of byte 1, others are not sent
		//Add the milesTag 'start' signal to the RMT buffer
		symbols_to_transmit_[index][0].duration0 = tx_start_on_time_;
		symbols_to_transmit_[index][0].level0 = 1;
		symbols_to_transmit_[index][0].duration1 = tx_off_time_;
		symbols_to_transmit_[index][0].level1 = 0;
		//Continue filling the buffer after the 'start' signal
		uint16_t bitIndex = 1;
		for(uint8_t bufferByteIndex = 0; bufferByteIndex < 4; bufferByteIndex++)
		{
			for(int8_t bufferBitIndex = 7; bufferBitIndex > -1; bufferBitIndex--)
			{
				if(bitIndex < maximum_number_of_symbols_)
				{
					if(debug_uart_ != nullptr)
					{
					  debug_uart_->printf_P(PSTR("milesTag: byte: %u Bit: %u - "), bufferByteIndex, bufferBitIndex);
					}
					if(bitRead(dataToSend[bufferByteIndex], bufferBitIndex))
					{
						if(debug_uart_ != nullptr)
						{
						  debug_uart_->print(F("1\r\n"));
						}
						symbols_to_transmit_[index][bitIndex].duration0 = tx_one_on_time_;
						symbols_to_transmit_[index][bitIndex].level0 = 1;
						symbols_to_transmit_[index][bitIndex].duration1 = tx_off_time_;
						symbols_to_transmit_[index][bitIndex].level1 = 0;
					}
					else
					{
						if(debug_uart_ != nullptr)
						{
						  debug_uart_->print(F("0\r\n"));
						}
						symbols_to_transmit_[index][bitIndex].duration0 = tx_zero_on_time_;
						symbols_to_transmit_[index][bitIndex].level0 = 1;
						symbols_to_transmit_[index][bitIndex].duration1 = tx_off_time_;
						symbols_to_transmit_[index][bitIndex].level1 = 0;
					}
				}
				bitIndex++;
			}
		}
		number_of_symbols_to_transmit_[index] = bitIndex;
	}
	uint8_t milesTagClass::map_damage_to_bitmask_(uint8_t damage)
	{
		switch(damage)
		{
			case 0 ... 1:
				return B00000000;
				break;
			case 2 ... 3:
				return B00000001;
				break;
			case 4:
				return B00000010;
				break;
			case 5 ... 6:
				return B00000011;
				break;
			case 7 ... 9:
				return B00000100;
				break;
			case 10 ... 14:
				return B00000101;
				break;
			case 15 ... 16:
				return B00000110;
				break;
			case 17 ... 19:
				return B00000111;
				break;
			case 20 ... 24:
				return B00001000;
				break;
			case 25 ... 29:
				return B00001001;
				break;
			case 30 ... 34:
				return B00001010;
				break;
			case 35 ... 39:
				return B00001011;
				break;
			case 40 ... 49:
				return B00001100;
				break;
			case 50 ... 74:
				return B00001101;
				break;
			case 75 ... 99:
				return B00001110;
				break;
			case 100:
				return B00001111;
				break;
			default:
				return B00000000;
				break;
		}
		return B00000000;
	}
	bool milesTagClass::transmit_stored_buffer_(uint8_t transmitterIndex, rmt_symbol_word_t* buffer, uint8_t bufferLength, bool wait)	//Transmit a buffer from the specified transmitter channel
	{
		if(debug_uart_ != nullptr)
		{
			debug_uart_->printf_P(PSTR("milesTag: sending %u symbols on channel %u\r\n"), bufferLength, transmitterIndex);
			for(uint8_t index = 0; index < bufferLength; index++)
			{
				debug_uart_->printf_P(PSTR("milesTag: symbol %02u - %s:%04u/%s:%04u\r\n"), index, (buffer[index].level0 == 0 ? "Off":"On"), buffer[index].duration0, (buffer[index].level1 == 0 ? "Off":"On"), buffer[index].duration1);
			}
		}
		uint32_t sendStart = micros();
		esp_err_t result = rmt_transmit(infrared_transmitter_handle_[transmitterIndex], copy_encoder_, buffer, bufferLength*sizeof(rmt_symbol_word_t), &event_transmitter_config_);	
		if(wait == true)	//Block until transmitted
		{
			rmt_tx_wait_all_done(infrared_transmitter_handle_[transmitterIndex], 1000);
		}
		uint32_t sendEnd = micros();
		if(result == ESP_OK)
		{
			if(debug_uart_ != nullptr)
			{
				debug_uart_->printf_P(PSTR("milesTag: queued data for transmitter %u in %u microseconds \r\n"), transmitterIndex, sendEnd - sendStart);
			}
			return true;
		}
		else
		{
			if(debug_uart_ != nullptr)
			{
				debug_uart_->printf_P(PSTR("RMT: failed to transmit from transmitter %u\r\n"), transmitterIndex);
			}
		}
		return false;
	}
	bool milesTagClass::transmitDamage(uint8_t damage, uint8_t transmitterIndex, bool wait)	//Send damage on the specified transmitter, defaults to 1 damage on the first
	{
		if(transmitters_configured_ == true)
		{
			if(number_of_symbols_to_transmit_[transmitterIndex] == 0)
			{
				if(debug_uart_ != nullptr)
				{
					debug_uart_->printf_P(PSTR("milesTag: transmitting %u damage on transmitter %u\r\n"), damage, transmitterIndex);
				}
				populate_buffer_with_damage_data_(transmitterIndex, damage);
				return transmit_stored_buffer_(transmitterIndex, symbols_to_transmit_[transmitterIndex], number_of_symbols_to_transmit_[transmitterIndex], wait);
			}
			else
			{
				if(debug_uart_ != nullptr)
				{
					debug_uart_->printf_P(PSTR("milesTag: transmitter %u busy\r\n"), transmitterIndex);
				}
			}
		}
		else
		{
			if(debug_uart_ != nullptr)
			{
				debug_uart_->print(F("Transmitters not initialised\r\n"));
			}
		}
		return false;
	}
#endif
#if defined SUPPORT_MILESTAG_RECEIVE
	bool milesTagClass::setReceivePin(int8_t pin, bool inverted)	//Set receive pin for a single transmitter device
	{
		if((type == deviceType::receiver || type == deviceType::combo) && infrared_receiver_config_ != nullptr)
		{
			return configure_rx_pin_(0, pin, inverted);
		}
		return false;
	}
	bool milesTagClass::setReceivePins(int8_t* pins)	//Set receive pins for a multi-receiver device
	{
		if((type == deviceType::receiver || type == deviceType::combo) && infrared_receiver_config_ != nullptr)
		{
			uint8_t successfullyConfigured = 0;
			for(uint8_t index = 0; index < number_of_receivers_; index++)
			{
				successfullyConfigured += (configure_rx_pin_(index, pins[index]) == true);
			}
			receivers_configured_ = (successfullyConfigured == number_of_receivers_);
			return transmitters_configured_;
		}
		return false;
	}
	bool rmt_rx_done_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
	{
		//milesTag.ir_data_received_ = true;
		*(uint8_t *)(user_data) = edata->num_symbols;
		return false;
	}
	/*
	bool milesTagClass::rx_done_callback_(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
	{
		milesTag.ir_data_received_ = true;
		milesTag.len = edata->num_symbols;
		BaseType_t high_task_wakeup = pdFALSE;
		return high_task_wakeup == pdTRUE;
	}
	*/
	bool milesTagClass::configure_rx_pin_(uint8_t index, int8_t pin, bool inverted)
	{
		infrared_receiver_config_[index] = {
			.gpio_num = static_cast<gpio_num_t>(pin),
			.clk_src = RMT_CLK_SRC_DEFAULT,
			.resolution_hz = 1000000,
			.mem_block_symbols = maximum_number_of_symbols_,
		};
		infrared_receiver_config_[index].flags = {
			.invert_in = inverted,
			.with_dma = false,
		};
		if(rmt_new_rx_channel(&infrared_receiver_config_[index], &infrared_receiver_handle_[index]) == ESP_OK)
		{
			rmt_rx_event_callbacks_t receive_callbacks_ = {
                .on_recv_done = rmt_rx_done_callback
				//.on_recv_done = rx_done_callback_
            };
			rmt_rx_register_event_callbacks(infrared_receiver_handle_[index], &receive_callbacks_, &number_of_received_symbols_[index]);
			rmt_enable(infrared_receiver_handle_[index]);
			rmt_receive(infrared_receiver_handle_[index], received_symbols_[index], maximum_number_of_symbols_*sizeof(rmt_symbol_word_t), &global_receiver_config_);
			if(debug_uart_ != nullptr)
			{
				debug_uart_->printf_P(PSTR("milesTag: configured pin %u for RX\r\n"), pin);
			}
			return true;
		}
		else
		{
			if(debug_uart_ != nullptr)
			{
				debug_uart_->printf_P(PSTR("milesTag: failed to configure pin %u for RX\r\n"), pin);
			}
		}
		return false;
	}
	bool milesTagClass::dataReceived()
	{
		for(uint8_t index = 0; index < number_of_receivers_; index++)
		{
			if(number_of_received_symbols_[index] > 0)
			{
				if(debug_uart_ != nullptr)
				{
					debug_uart_->printf_P(PSTR("milesTag: received %u symbols on channel %u\r\n"), number_of_received_symbols_[index], index);//index);
					for(uint16_t i=0;i<number_of_received_symbols_[index];i++)
					{
						debug_uart_->printf("milesTag: symbol %02u - %s:%04u/%s:%04u\r\n", i,!received_symbols_[index][i].level0 ? "Off":"On", received_symbols_[index][i].duration0,!received_symbols_[index][i].level1 ? "Off":"On", received_symbols_[index][i].duration1);
					}
				}
				return true;
			}
		}
		return false;
	}
	bool milesTagClass::resumeReception()
	{
		for(uint8_t index = 0; index < number_of_receivers_; index++)
		{
			if(number_of_received_symbols_[index] > 0)
			{
				if(debug_uart_ != nullptr)
				{
					debug_uart_->printf_P(PSTR("milesTag: resuming reception on channel %u\r\n"), index);
				}
				number_of_received_symbols_[index] = 0;
				rmt_receive(infrared_receiver_handle_[index], received_symbols_[index], maximum_number_of_symbols_*sizeof(rmt_symbol_word_t), &global_receiver_config_);
				return true;
			}
		}
		return false;
	}
	uint8_t milesTagClass::map_bitmask_to_damage_(uint8_t bitmask)
	{
		switch(bitmask)
		{
			case 0:
				return 1;
				break;
			case 1:
				return 2;
				break;
			case 2:
				return 4;
				break;
			case 3:
				return 5;
				break;
			case 4:
				return 7;
				break;
			case 5:
				return 10;
				break;
			case 6:
				return 15;
				break;
			case 7:
				return 17;
				break;
			case 8:
				return 20;
				break;
			case 9:
				return 25;
				break;
			case 10:
				return 30;
				break;
			case 11:
				return 35;
				break;
			case 12:
				return 40;
				break;
			case 13:
				return 50;
				break;
			case 14:
				return 75;
				break;
			case 15:
				return 100;
				break;
			default:
				return 1;
				break;
		}
		return 1;
	}
#endif
/*
gpio_num_t milesTagClass::int8_t_to_gpio_num_t(int8_t pin)
{
	return (gpio_num_t) pin;
}
*/
void milesTagClass::setPlayerId(uint8_t id)	//Set the player ID, which can be 0-127, default 1
{
	player_id_ = id;
}
void milesTagClass::setTeamId(uint8_t id)	//Set the player team ID, which can be 0-3, default 0
{
	team_id_ = id;
}
void milesTagClass::debug(Stream &terminalStream)
{
	debug_uart_ = &terminalStream;		//Set the stream used for the terminal
	#if defined(ESP8266)
	if(&terminalStream == &Serial)
	{
		  debug_uart_->write(17);			//Send an XON to stop the hung terminal after reset on ESP8266
	}
	#endif
	if(debug_uart_ != nullptr)
	{
		debug_uart_->print(F("milesTag: debug enabled\r\n"));
	}
}
milesTagClass milesTag;	//Create an instance of the class, as only one is practically usable at a time

#endif
