#include "ST7565Emu.h"

namespace HaH
{
	namespace RC
	{

ST7565EmuBase::ST7565EmuBase(TFT_eSPI& display):
	display(display), left(0), top(0), base_y(0), current_x(0)
{
	memset(trans, 0, sizeof(trans));
	for(uint i = 0; i < QUEUE_SIZE; ++i)
	{
		trans[i].length = sizeof(rcv_buffer[i]) * 8;
		trans[i].rx_buffer = &rcv_buffer[i];
	}

	memset(display_buffer, BG, sizeof(display_buffer));
}

void ST7565EmuBase::run()
{
	left = (display.width() - COLUMNS_USED) / 2;
	top = (display.height() - ROWS_USED) / 2;

	//Ring buffer start position and size
	uint start = 0, size = 0;

	while(true)
	{
		//Fill transaction queue
		while(size < QUEUE_SIZE)
		{
			spi_slave_queue_trans(HSPI_HOST, &trans[(start + size) % QUEUE_SIZE], portMAX_DELAY);
			++size;
		}
		
		spi_slave_transaction_t* t = nullptr;
		if(ESP_OK == spi_slave_get_trans_result(HSPI_HOST, &t, portMAX_DELAY))
		{
			uint8_t received = rcv_buffer[start];
			if(trans[start].user)
				do_display_data(received);
			else
				do_command(received);
		}

		//Remove current transaction from buffer
		--size;
		start = ++start % QUEUE_SIZE;
	}
}

inline void ST7565EmuBase::do_command(uint8_t cmd)
{
	uint8_t data = 0x0Fu & cmd;
	switch(0xF0u & cmd)
	{
		//Set page
		case 0b10110000u:
			if(PAGES_USED > data)
			{
				base_y = data * DISPLAY_ROWS_PER_PAGE * COLUMNS_USED;
				//There are glitches when doing separate set column commands
				//but the KK firmware anyway sets column to 0 after every page command
				current_x = 0;
			}
			break;

		/*
		//Set column low
		case 0b00000000u:
			current_x = (0xF0u & current_x) | data;
			break;
		
		//Set column high
		case 0b00010000u:
			current_x = (0x0Fu & current_x) | (data << 4);
			break;
		*/
	}
}

inline void ST7565EmuBase::do_display_data(uint8_t data)
{
	uint y = base_y;
	for(uint row = 0; row < DISPLAY_ROWS_PER_PAGE; ++row)
	{
		uint pos = y + current_x;
		if(TRIGGERPOS_SHOW > pos)
		{
			display_buffer[pos] = (0x01u & data) ? FG : BG;
			data >>= 1;
			y += COLUMNS_USED;
		}
		else
		{
			/*
				Since there is no end of image command or signal 
				draw whole image after the last pixel was transmitted.
				This is taylored specifically to the KK firmware.
			*/
			display_buffer[TRIGGERPOS_SHOW] = (0x01u & data) ? FG : BG;
			//Prevents glitches in the next frame because drawing is slow
			base_y = current_x = 0;
			draw();

			return;
		}
	}
	++current_x;
}

void ST7565EmuBase::draw()
{
	display.pushImage(left, top, COLUMNS_USED, ROWS_USED, display_buffer);
	vTaskDelay(1); //Got device resets without due to an "idle task" watchdog
}

	} //NS RC
} //NS HaH
