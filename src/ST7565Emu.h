#pragma once

#ifndef ST7565EMU_H_5CFB9D75_F0EB_4155_A2A2_9171F048D05C
#define ST7565EMU_H_5CFB9D75_F0EB_4155_A2A2_9171F048D05C

#include <driver/gpio.h>
#include <driver/spi_slave.h>
#include <TFT_eSPI.h>

namespace HaH
{
	namespace RC
	{

//Little helper for colors
constexpr uint16_t byteswap(uint16_t val) noexcept
{
	return (val >> 8) | ((val & 0x00FFu) << 8);
}

class ST7565EmuBase
{
public:
	//ST7565 can handle:
	static const uint DISPLAY_ROWS = 65, DISPLAY_COLUMNS = 132, DISPLAY_PAGES = 9, DISPLAY_ROWS_PER_PAGE = 8;
	static const uint DISPLAY_PAGE_SIZE = DISPLAY_ROWS_PER_PAGE * DISPLAY_COLUMNS, DISPLAY_SIZE = DISPLAY_PAGE_SIZE * DISPLAY_PAGES;

	//KK with Steveis firmware through V1.19S1PRO uses only 128x64 of display
	static const uint KK_COLUMNS_USED = 128, KK_ROWS_USED = 64, KK_PAGES_USED = 8;
	static const uint KK_BUFFER_SIZE = KK_COLUMNS_USED * KK_ROWS_USED;
	
	//Emulate the look of the KK Programmer LCD, light grey for background, dark grey for pixels.
	//Low and high byte of the TFT_* color constants need to be swapped.
	static const uint16_t KK_BG = byteswap(TFT_LIGHTGREY), KK_FG = byteswap(TFT_DARKGREY);

	//Set the config of the actual display to emulate
	static const uint	COLUMNS_USED = KK_COLUMNS_USED,
						ROWS_USED = KK_ROWS_USED,
						PAGES_USED = KK_PAGES_USED,
						BUFFER_SIZE = KK_BUFFER_SIZE,
						TRIGGERPOS_SHOW = BUFFER_SIZE - 1;
	static const uint16_t FG = KK_FG, BG = KK_BG;

public:
	ST7565EmuBase(TFT_eSPI& display);
	ST7565EmuBase(const ST7565EmuBase&) = delete;
	ST7565EmuBase& operator=(const ST7565EmuBase&) = delete;
	
	void run();

protected:
	//SPI transaction queue
	static const uint QUEUE_SIZE = 64; //Largest possible size on ESP32 with RTOS

private:
	TFT_eSPI& display;
	uint16_t left, top;
	
	//Ring buffer of transactions to insert into the SPI queue
	WORD_ALIGNED_ATTR spi_slave_transaction_t trans[QUEUE_SIZE];
	WORD_ALIGNED_ATTR uint8_t rcv_buffer[QUEUE_SIZE];
	
	//Display data
	WORD_ALIGNED_ATTR uint base_y, current_x;
	WORD_ALIGNED_ATTR uint16_t display_buffer[BUFFER_SIZE];

private:
	inline void do_command(uint8_t cmd);
	inline void do_display_data(uint8_t data);
	void draw();
};

template<gpio_num_t pin_sclk, gpio_num_t pin_mosi, gpio_num_t pin_csl, gpio_num_t pin_a0> class ST7565Emu: public ST7565EmuBase
{
public:
	ST7565Emu(TFT_eSPI& display):
		ST7565EmuBase(display)
	{
		pinMode(pin_sclk, INPUT_PULLUP);
		pinMode(pin_mosi, INPUT_PULLUP);
		pinMode(pin_csl, INPUT_PULLUP);
		pinMode(pin_a0, INPUT_PULLUP);

		spi_bus_config_t bus;
		memset(&bus, 0, sizeof(bus));
		bus.sclk_io_num = pin_sclk;
		bus.mosi_io_num = pin_mosi;
		bus.miso_io_num = -1;
		bus.quadwp_io_num = -1;
		bus.quadhd_io_num = -1;
		
		spi_slave_interface_config_t slave;
		memset(&slave, 0, sizeof(slave));
		slave.mode = SPI_MODE3;
		slave.spics_io_num = pin_csl;
		slave.queue_size = QUEUE_SIZE;
		slave.post_setup_cb = nullptr;
		slave.post_trans_cb = post_trans_cb;

		spi_slave_initialize(HSPI_HOST, &bus, &slave, SPI_DMA_DISABLED);
	}

private:
	static void IRAM_ATTR post_setup_cb(spi_slave_transaction_t* t)
	{

	}

	/*
		The nearest sampling position of A0
		one can reach without doing SPI on your own.
		Luckily, it works with the KK firmware.
	*/
	static void IRAM_ATTR post_trans_cb(spi_slave_transaction_t* t)
	{
		t->user = reinterpret_cast<void*>(gpio_get_level(pin_a0));
	}
};

	} //NS RC
} //NS HaH

#endif