#include <Arduino.h>
#include "ST7565Emu.h"

const gpio_num_t KK_PIN_MOSI = GPIO_NUM_27, KK_PIN_SCLK = GPIO_NUM_26, KK_PIN_A0 = GPIO_NUM_25, KK_PIN_CSL = GPIO_NUM_33, KK_PIN_RST = GPIO_NUM_32 /* unused */;
const gpio_num_t INPUT_PIN_X = GPIO_NUM_37, INPUT_PIN_Y = GPIO_NUM_38;
const gpio_num_t OUTPUT_PIN_KEY1 = GPIO_NUM_12, OUTPUT_PIN_KEY2 = GPIO_NUM_13, OUTPUT_PIN_KEY3 = GPIO_NUM_15, OUTPUT_PIN_KEY4 = GPIO_NUM_2;


TFT_eSPI tft;
HaH::RC::ST7565Emu<KK_PIN_SCLK, KK_PIN_MOSI, KK_PIN_CSL, KK_PIN_A0> emu(tft);

void keys(void* p)
{
	while(true)
	{
		//Poll joytick for key emulation
		auto x = analogRead(INPUT_PIN_X);
		auto y = analogRead(INPUT_PIN_Y);
		
		//Active keys go low
		digitalWrite(OUTPUT_PIN_KEY2, x < 100 ? 0 : 1);
		digitalWrite(OUTPUT_PIN_KEY3, x > 4000 ? 0 : 1);
		digitalWrite(OUTPUT_PIN_KEY4, y < 100 ? 0 : 1);
		digitalWrite(OUTPUT_PIN_KEY1, y > 4000 ? 0 : 1);

		vTaskDelay(5);
	}
}

void display(void* p)
{
	emu.run();
}

void setup()
{
	Serial.begin(115200);

	pinMode(INPUT_PIN_X, INPUT_PULLUP);
	pinMode(INPUT_PIN_Y, INPUT_PULLUP);
	pinMode(OUTPUT_PIN_KEY1, OUTPUT);
	pinMode(OUTPUT_PIN_KEY2, OUTPUT);
	pinMode(OUTPUT_PIN_KEY3, OUTPUT);
	pinMode(OUTPUT_PIN_KEY4, OUTPUT);

	tft.init();
	tft.setRotation(3);
	tft.fillScreen(TFT_BLACK);

	//Offload the display task to the other core
	xTaskCreatePinnedToCore(display, "Display", 4096, nullptr, configMAX_PRIORITIES - 1, nullptr, xPortGetCoreID() == 0 ? 1 : 0);
}

void loop(void)
{
	keys(nullptr);
}
