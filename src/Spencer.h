#include <Arduino.h>
#include <SPI.h>
#include <SerialFlash.h>
#include <CircuitOS.h>
#include <Input/InputGPIO.h>
#include "Speech/SpeechToIntent.h"
#include "Audio/Playback.h"
#include <Devices/LEDmatrix/LEDmatrix.h>
#include <Loop/LoopManager.h>
#include <WiFi.h>
#include "Audio/Recording.h"
#include <Util/Task.h>
#include "Settings.h"
#include <Network/Net.h>
#include <Devices/LEDmatrix/SerialFlashFileAdapter.h>

#ifndef SPENCER_HPP
#define SPENCER_HPP

#define NEOPIXEL_PIN 2
#define FLASH_CS_PIN 5
#define BTN_PIN 17
#define LED_PIN 26

class SpencerImpl
{
public:
	SpencerImpl();
	~SpencerImpl();
	void begin();
};

extern LEDmatrixImpl LEDmatrix;
extern SpencerImpl Spencer;
#endif
