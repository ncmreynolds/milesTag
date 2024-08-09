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
void milesTagClass::setCarrierFrequency(uint16_t frequency)	//Must be done before begin(), default is 56000
{
	carrier_frequency_ = frequency;
}
void milesTagClass::setDutyCycle(uint8_t duty, uint8_t transmitterIndex)	//Must be done before begin(), default is 50 and very unlikely to change
{
	carrier_duty_cycle_ = duty;
}
bool milesTagClass::begin(deviceType typeToIntialise, uint8_t numberOfTransmitters, uint8_t numberOfReceivers) 
{
	type = typeToIntialise;
	bool initialisation_success_ = true;
	if(type == deviceType::combo)
	{
		if(numberOfTransmitters + numberOfReceivers + 1 > RMT_CHANNEL_MAX)
		{
			initialisation_success_ = false;
		}
	}
	else if(numberOfTransmitters + 1 > RMT_CHANNEL_MAX)
	{
		initialisation_success_ = false;
	}
	else if(numberOfReceivers + 1 > RMT_CHANNEL_MAX)
	{
		initialisation_success_ = false;
	}
	if(initialisation_success_ == false)
	{
		if(debug_uart_ != nullptr)
		{
			debug_uart_->println(F("milesTag: too many channels requested"));
		}
		return initialisation_success_;
	}
	periph_module_disable(PERIPH_RMT_MODULE);
    periph_module_enable(PERIPH_RMT_MODULE);
	if(type == deviceType::transmitter || type == deviceType::combo)
	{
		number_of_transmitters_ = numberOfTransmitters;
		infrared_transmitter_ = new rmt_config_t[number_of_transmitters_];
		tx_buffer_ = new rmt_item32_t*[number_of_transmitters_];
		for(uint8_t index = 0; index < tx_length_; index++)
		{
			tx_buffer_[index] = new rmt_item32_t[tx_length_];
		}
		if(debug_uart_ != nullptr)
		{
			debug_uart_->printf_P(PSTR("milesTag: created %u TX channel(s)\r\n"), number_of_transmitters_);
		}
	}
	if(type == deviceType::receiver || type == deviceType::combo)
	{
		number_of_receivers_ = numberOfReceivers;
		infrared_receiver_ = new rmt_config_t[number_of_receivers_];
		if(debug_uart_ != nullptr)
		{
			debug_uart_->printf_P(PSTR("milesTag: created %u RX channel(s)\r\n"), number_of_receivers_);
		}
	}
	return initialisation_success_;
}
bool milesTagClass::setTransmitPin(int8_t pin)	//Set transmit pin for a single transmitter device
{
	if((type == deviceType::transmitter || type == deviceType::combo) && infrared_transmitter_ != nullptr)
	{
		transmitters_configured_ = configure_tx_pin_(0, pin);
		return transmitters_configured_;
	}
	return false;
}
bool milesTagClass::setTransmitPins(int8_t* pins)	//Set transmit pins for a multi-transmitter device
{
	if((type == deviceType::transmitter || type == deviceType::combo) && infrared_transmitter_ != nullptr)
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
bool milesTagClass::configure_tx_pin_(uint8_t index, int8_t pin)
{
	pinMode(pin,OUTPUT);
	infrared_transmitter_[index].rmt_mode = RMT_MODE_TX;								//Set as transmitter
	infrared_transmitter_[index].channel = index_to_channel_(current_free_channel_++);	//Use the current free channel and increment it
	infrared_transmitter_[index].gpio_num = int8_t_to_gpio_num_t(pin);					//Set the pin
	infrared_transmitter_[index].mem_block_num = current_free_memory_block_++;			//Use the current free block and increment it
	infrared_transmitter_[index].tx_config.carrier_freq_hz = carrier_frequency_;		//Set carrier frequency
	infrared_transmitter_[index].tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;		//Set up carrier, it's rarely going to be inverted
	infrared_transmitter_[index].tx_config.idle_level = RMT_IDLE_LEVEL_LOW;				//Set up carrier, it's rarely going to be inverted
	infrared_transmitter_[index].tx_config.carrier_duty_percent = carrier_duty_cycle_;	//Dynamically adjustable duty cycle may be a future development
	infrared_transmitter_[index].tx_config.carrier_en = 1;								//Definitely need a carrier
	infrared_transmitter_[index].tx_config.loop_en = 0;									//No looping needed, milesTag is normally one-shot
	infrared_transmitter_[index].tx_config.idle_output_en = 1;							//Enforce output state instead of relying on external pullup/down
	infrared_transmitter_[index].clk_div = 80;											//80MHz / 80 = 1MHz 0r 1uS per count, used to define the symbols, not related to CPU speed
	rmt_config(&infrared_transmitter_[index]);											//Load the RMT config
	if(rmt_driver_install(infrared_transmitter_[index].channel, 0, 0) == ESP_OK)		//Install the RMT driver
	{
		if(debug_uart_ != nullptr)
		{
			debug_uart_->printf_P(PSTR("milesTag: configured pin %u as TX %u at %.2fKHz %u%% duty cycle\r\n"), pin, index, (float)carrier_frequency_/1000.0, carrier_duty_cycle_);
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
void milesTagClass::populate_buffer_with_damage_data_(uint8_t transmitterIndex, uint8_t damage)
{
	uint8_t dataToSend[4] = {0,0};
	dataToSend[0] = player_id_ & B01111111;																		//Player ID is in bottom 7 bits of byte 0, the top bit is always zero for damage
	dataToSend[1] = ((uint8_t)team_id_) << 6;																	//Team ID is in top two bits of byte 1
	dataToSend[1] = dataToSend[1] | ((map_damage_to_bitmask_(damage)) << 2);									//Damage is in next four bits of byte 1, others are not sent
	//Add the milesTag 'start' signal to the RMT buffer
	tx_buffer_[transmitterIndex][0].duration0 = tx_start_on_time_/2;
	tx_buffer_[transmitterIndex][0].level0 = 1;
	tx_buffer_[transmitterIndex][0].duration1 = tx_start_on_time_/2;
	tx_buffer_[transmitterIndex][0].level1 = 1;
	//Continue filling the buffer after the 'start' signal
	uint16_t bitIndex = 1;
	for(uint8_t bufferByteIndex = 0; bufferByteIndex < 4; bufferByteIndex++)
	{
		for(int8_t bufferBitIndex = 7; bufferBitIndex > -1; bufferBitIndex--)
		{
			if(bitIndex < tx_length_)
			{
				if(debug_uart_ != nullptr)
				{
				  debug_uart_->printf_P(PSTR("milestag: byte: %u Bit: %u - "), bufferByteIndex, bufferBitIndex);
				}
				if(bitRead(dataToSend[bufferByteIndex], bufferBitIndex))
				{
					if(debug_uart_ != nullptr)
					{
					  debug_uart_->print(F("1\r\n"));
					}
					tx_buffer_[transmitterIndex][bitIndex].duration0 = tx_off_time_;
					tx_buffer_[transmitterIndex][bitIndex].level0 = 0;
					tx_buffer_[transmitterIndex][bitIndex].duration1 = tx_one_on_time_;
					tx_buffer_[transmitterIndex][bitIndex].level1 = 1;
				}
				else
				{
					if(debug_uart_ != nullptr)
					{
					  debug_uart_->print(F("0\r\n"));
					}
					tx_buffer_[transmitterIndex][bitIndex].duration0 = tx_off_time_;
					tx_buffer_[transmitterIndex][bitIndex].level0 = 0;
					tx_buffer_[transmitterIndex][bitIndex].duration1 = tx_zero_on_time_;
					tx_buffer_[transmitterIndex][bitIndex].level1 = 1;
				}
			}
			bitIndex++;
		}
	}
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
bool milesTagClass::transmit_stored_buffer_(uint8_t transmitterIndex, rmt_item32_t* buffer, uint8_t bufferLength)	//Transmit a buffer from the specified transmitter channel
{
	if(debug_uart_ != nullptr)
	{
		debug_uart_->print(F("RMT: Sending durations\r\n"));
		for(uint8_t index = 0; index < bufferLength; index++)
		{
			debug_uart_->printf_P(PSTR("RMT: %04u %s/%04u %s\r\n"), buffer[index].duration0, (buffer[index].level0 == 0 ? "off":"on"), buffer[index].duration1, (buffer[index].level1 == 0? "off":"on"));
		}
	}
	uint32_t sendStart = micros();
	esp_err_t result = rmt_write_items(infrared_transmitter_[transmitterIndex].channel, buffer, bufferLength, false);	//Transmit asynchronously
	uint32_t sendEnd = micros();
	if(result == ESP_OK)
	{
		if(debug_uart_ != nullptr)
		{
			debug_uart_->printf_P(PSTR("RMT: queued data in %u microseconds from transmitter %u\r\n"), sendEnd - sendStart, transmitterIndex);
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
bool milesTagClass::setReceivePin(int8_t pin)	//Set receive pin for a single transmitter device
{
	if((type == deviceType::receiver || type == deviceType::combo) && infrared_receiver_ != nullptr)
	{
		return configure_rx_pin_(0, pin);
	}
	return false;
}
bool milesTagClass::setReceivePins(int8_t* pins)	//Set receive pins for a multi-receiver device
{
	if((type == deviceType::receiver || type == deviceType::combo) && infrared_receiver_ != nullptr)
	{
		for(uint8_t index = 0; index < number_of_receivers_; index++)
		{
			configure_rx_pin_(index, pins[index]);
		}
		return true;
	}
	return false;
}
bool milesTagClass::configure_rx_pin_(uint8_t index, int8_t pin)
{
	infrared_transmitter_[index].rmt_mode = RMT_MODE_TX;								//Set as transmitter
	infrared_transmitter_[index].channel = index_to_channel_(current_free_channel_++);	//Use the current free channel and increment
	infrared_transmitter_[index].gpio_num = int8_t_to_gpio_num_t(pin);					//Set the pin
	infrared_transmitter_[index].mem_block_num = 1;										//Set a memory block
	infrared_transmitter_[index].tx_config.loop_en = 0;									//No looping needed, milesTag is normally one-shot
	infrared_transmitter_[index].tx_config.carrier_en = 1;								//Definitely need a carrier
	infrared_transmitter_[index].tx_config.idle_output_en = 1;							
	infrared_transmitter_[index].tx_config.carrier_freq_hz = carrier_frequency_;		//Set carrier frequency
	infrared_transmitter_[index].tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;		//Set up carrier, it's rarely going to be inverted
	infrared_transmitter_[index].tx_config.idle_level = RMT_IDLE_LEVEL_LOW;				//Set up carrier, it's rarely going to be inverted
	infrared_transmitter_[index].tx_config.carrier_duty_percent = carrier_duty_cycle_;	//Dynamically adjustable duty cycle may be a future development
	infrared_transmitter_[index].clk_div = 80;											// 80MHz / 80 = 1MHz 0r 1uS per count, used to define the symbols, not related to CPU speed
	if(rmt_driver_install(infrared_transmitter_[index].channel, 0, 0) == ESP_OK)		//Install the RMT driver
	{
		if(debug_uart_ != nullptr)
		{
			debug_uart_->printf_P(PSTR("milesTag: configured pin %u for RX at %f.2KHz\r\n"), pin, (float)carrier_frequency_/1000.0);
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
rmt_channel_t milesTagClass::index_to_channel_(uint8_t index)	//Maps an integer index to an RMT channel
{
	switch(index)
	{
		#if ESP_IDF_VERSION_MAJOR > 3
			#if CONFIG_IDF_TARGET_ESP32
				case 0:
					return RMT_CHANNEL_0;
					break;
				case 1:
					return RMT_CHANNEL_1;
					break;
				case 2:
					return RMT_CHANNEL_2;
					break;
				case 3:
					return RMT_CHANNEL_3;
					break;
				case 4:
					return RMT_CHANNEL_4;
					break;
				case 5:
					return RMT_CHANNEL_5;
					break;
				case 6:
					return RMT_CHANNEL_6;
					break;
				case 7:
					return RMT_CHANNEL_7;
					break;
			#elif CONFIG_IDF_TARGET_ESP32S2
				case 0:
					return RMT_CHANNEL_0;
					break;
				case 1:
					return RMT_CHANNEL_1;
					break;
				case 2:
					return RMT_CHANNEL_2;
					break;
				case 3:
					return RMT_CHANNEL_3;
					break;
			#elif CONFIG_IDF_TARGET_ESP32S3
			#elif CONFIG_IDF_TARGET_ESP32C3
			#else 
				#error Target CONFIG_IDF_TARGET is not supported
			#endif
		#else // ESP32 Before IDF 4.0
			#include "rom/rtc.h"
		#endif
	}
	return RMT_CHANNEL_MAX;
}
gpio_num_t milesTagClass::int8_t_to_gpio_num_t(int8_t pin)
{
	return (gpio_num_t) pin;
}
bool milesTagClass::transmitDamage(uint8_t damage, uint8_t transmitterIndex)	//Send damage on the specified transmitter, defaults to 1 damage on the first
{
	if(transmitters_configured_ == true)
	{
		if(debug_uart_ != nullptr)
		{
			debug_uart_->printf_P(PSTR("milesTag: transmitting %u damage on transmitter %u\r\n"), damage, transmitterIndex);
		}
		populate_buffer_with_damage_data_(transmitterIndex, damage);
		return transmit_stored_buffer_(transmitterIndex, tx_buffer_[transmitterIndex], tx_length_);
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
}
milesTagClass milesTag;	//Create an instance of the class, as only one is practically usable at a time
#endif
