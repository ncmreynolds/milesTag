#ifndef esp32rmtHelpers_cpp
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
#include <esp32rmtHelpers.h>
esp32rmtTransmitHelper::esp32rmtTransmitHelper()	//Constructor function
{
}

esp32rmtTransmitHelper::~esp32rmtTransmitHelper()	//Destructor function
{
}
/*
 *
 * Non-class callback, ugh
 *
 */
bool esp32rmtTransmitHelperTxDoneCallback(rmt_channel_handle_t channel, const rmt_tx_done_event_data_t *edata, void *user_data)
{
	*(uint8_t *)(user_data) = 0;	//Reset the symbol count, which shows this channel as free
	return false;
}
/*
 *
 *	Initialise one or more transmitters
 *
 */
bool esp32rmtTransmitHelper::begin(uint8_t numberOfTransmitters)
{
	bool initialisation_success_ = true;
	number_of_transmitters_ = numberOfTransmitters;											//Record the number of transmitters
	infrared_transmitter_handle_ = new rmt_channel_handle_t[number_of_transmitters_];		//Create array of RMT handle(s)
	infrared_transmitter_config_ = new rmt_tx_channel_config_t[number_of_transmitters_];	//Create array of RMT configuration(s)
	symbols_to_transmit_ = new rmt_symbol_word_t*[number_of_transmitters_];					//Create array of symbol buffer(s)
	number_of_symbols_to_transmit_ = new uint8_t[number_of_transmitters_];					//Create array of symbol buffer length(s)
	if(rmt_new_copy_encoder(&copy_encoder_config_, &copy_encoder_) != ESP_OK)				//Initialise the copy encoder
	{
		if(debug_uart_ != nullptr)
		{
			debug_uart_->println(F("esp32rmtTransmitHelper: failed to create copy encoder"));
		}
		initialisation_success_ = false;
	}
	else
	{
		if(debug_uart_ != nullptr)
		{
			debug_uart_->println(F("esp32rmtTransmitHelper: created copy encoder"));
		}
	}
	for(uint8_t index = 0; index < number_of_transmitters_; index++)
	{
		symbols_to_transmit_[index] = new rmt_symbol_word_t[maximum_number_of_symbols_];
		number_of_symbols_to_transmit_[index] = 0;
	}
	return initialisation_success_;
}
void esp32rmtTransmitHelper::setCarrierFrequency(uint16_t frequency)				//Must be done before begin(), default is 56000
{
	global_transmitter_config_.frequency_hz = frequency;
}
void esp32rmtTransmitHelper::setDutyCycle(uint8_t duty, uint8_t transmitterIndex)	//Must be done before begin(), default is 50 and very unlikely to change
{
	global_transmitter_config_.duty_cycle = float(duty)/100.0;
}
void esp32rmtTransmitHelper::setMaximumNumberOfSymbols(uint8_t symbols)				//Must be done before begin(), default is 64
{
	if(symbols > 48)																//There is a minimum of 48
	{
		maximum_number_of_symbols_ = symbols;
	}
}
bool esp32rmtTransmitHelper::configure_tx_pin_(uint8_t index, int8_t pin)
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
			.on_trans_done = esp32rmtTransmitHelperTxDoneCallback
		};
		rmt_tx_register_event_callbacks(infrared_transmitter_handle_[index], &transmit_callbacks_, &number_of_symbols_to_transmit_[index]);
		rmt_apply_carrier(infrared_transmitter_handle_[index], &global_transmitter_config_);
		rmt_enable(infrared_transmitter_handle_[index]);
		if(debug_uart_ != nullptr)
		{
			debug_uart_->printf_P(PSTR("esp32rmtTransmitHelper: configured pin %u as transmitter %u at %.2fKHz %u%% duty cycle\r\n"), pin, index, float(global_transmitter_config_.frequency_hz/1000), uint8_t(global_transmitter_config_.duty_cycle*100));
		}
		return true;
	}
	else
	{
		if(debug_uart_ != nullptr)
		{
			debug_uart_->printf_P(PSTR("esp32rmtTransmitHelper: failed to configure pin %u for TX\r\n"), pin);
		}
	}
	return false;
}
bool esp32rmtTransmitHelper::transmitterBusy(uint8_t index)
{
	return number_of_symbols_to_transmit_[index] != 0;
}
uint8_t esp32rmtTransmitHelper::maximumNumberOfSymbols()	//Maximum number of symbols
{
	return maximum_number_of_symbols_;
}
bool esp32rmtTransmitHelper::addSymbol(uint8_t index, uint16_t duration0, uint8_t level0, uint16_t duration1, uint8_t level1)
{
	if(number_of_symbols_to_transmit_[index] < maximum_number_of_symbols_)
	{
		symbols_to_transmit_[index][number_of_symbols_to_transmit_[index]].duration0 = duration0;
		symbols_to_transmit_[index][number_of_symbols_to_transmit_[index]].level0 = level0;
		symbols_to_transmit_[index][number_of_symbols_to_transmit_[index]].duration1 = duration1;
		symbols_to_transmit_[index][number_of_symbols_to_transmit_[index]].level1 = level1;
		/*
		if(debug_uart_ != nullptr)
		{
			debug_uart_->printf_P(PSTR("esp32rmtTransmitHelper: Transmitter %u added symbol %u %u/%u %u/%u\r\n"), index, number_of_symbols_to_transmit_[index], duration0, level0, duration1, level1);
		}
		*/
		number_of_symbols_to_transmit_[index] = number_of_symbols_to_transmit_[index] + 1;
		return true;
	}
	return false;
}
bool esp32rmtTransmitHelper::transmit_stored_buffer_(uint8_t transmitterIndex, bool wait)	//Transmit a buffer from the specified transmitter channel
{
	if(debug_uart_ != nullptr)
	{
		debug_uart_->printf_P(PSTR("esp32rmtTransmitHelper: sending %u symbols on channel %u wait = %u\r\n"), number_of_symbols_to_transmit_[transmitterIndex], transmitterIndex, wait);
		for(uint8_t index = 0; index < number_of_symbols_to_transmit_[transmitterIndex]; index++)
		{
			debug_uart_->printf_P(PSTR("esp32rmtTransmitHelper: symbol %02u - %s:%04u/%s:%04u\r\n"), index, (symbols_to_transmit_[transmitterIndex][index].level0 == 0 ? "Off":"On"), symbols_to_transmit_[transmitterIndex][index].duration0, (symbols_to_transmit_[transmitterIndex][index].level1 == 0 ? "Off":"On"), symbols_to_transmit_[transmitterIndex][index].duration1);
		}
	}
	//number_of_symbols_to_transmit_[transmitterIndex] = 0;
	//return true;
	uint32_t sendStart = micros();
	esp_err_t result = rmt_transmit(infrared_transmitter_handle_[transmitterIndex], copy_encoder_, symbols_to_transmit_[transmitterIndex], number_of_symbols_to_transmit_[transmitterIndex]*sizeof(rmt_symbol_word_t), &event_transmitter_config_);	
	if(wait == true)	//Block until transmitted
	{
		rmt_tx_wait_all_done(infrared_transmitter_handle_[transmitterIndex], 1000);
	}
	uint32_t sendEnd = micros();
	if(result == ESP_OK)
	{
		if(debug_uart_ != nullptr)
		{
			debug_uart_->printf_P(PSTR("esp32rmtTransmitHelper: queued data for transmitter %u in %u microseconds \r\n"), transmitterIndex, sendEnd - sendStart);
		}
		return true;
	}
	else
	{
		if(debug_uart_ != nullptr)
		{
			debug_uart_->printf_P(PSTR("esp32rmtTransmitHelper: failed to transmit from transmitter %u\r\n"), transmitterIndex);
		}
	}
	return false;
}
void esp32rmtTransmitHelper::debug(Stream &terminalStream)
{
	debug_uart_ = &terminalStream;		//Set the stream used for the terminal
	if(debug_uart_ != nullptr)
	{
		debug_uart_->print(F("esp32rmtTransmitHelper: debug enabled\r\n"));
	}
}
esp32rmtReceiveHelper::esp32rmtReceiveHelper()		//Constructor function
{
}
esp32rmtReceiveHelper::~esp32rmtReceiveHelper()		//Destructor function
{
}
/*
 *
 *	Initialise one or more receivers
 *
 */
bool esp32rmtReceiveHelper::begin(uint8_t numberOfReceivers)
{
	number_of_receivers_ = numberOfReceivers;											//Record the number of receivers
	infrared_receiver_handle_ = new rmt_channel_handle_t[number_of_receivers_];			//Create array of RMT handle(s)
	infrared_receiver_config_ = new rmt_rx_channel_config_t[number_of_receivers_];		//Create array of RMT configuration(s)	
	received_symbols_ = new rmt_symbol_word_t*[number_of_receivers_];					//Create array of symbol buffer(s)
	number_of_received_symbols_ = new uint8_t[number_of_receivers_];					//Create array of symbol buffer length(s)
	message_data_ = new uint8_t*[number_of_receivers_];									//Create array of data buffer(s)
	for(uint8_t index = 0; index < number_of_receivers_; index++)
	{
		received_symbols_[index] = new rmt_symbol_word_t[maximum_number_of_symbols_];	//Create symbol buffers
		number_of_received_symbols_[index] = 0;											//Set received buffer length to zero
		message_data_[index] = new uint8_t[maximum_message_length_];					//Create data buffer
	}
}
#endif