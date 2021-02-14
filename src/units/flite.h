#ifndef __FLITE_UNIT__H__
#define __FLITE_UNIT__H__

#include <Preferences.h>
#include <SHT31.h>
#include <Adafruit_VL53L0X.h>

#include "unit.h"

// https://www.flitesense.com/arduino-integration

#define XMA 1

class FliteUnit : public Unit {
  public:
  	FliteUnit(Host *host, uint8_t channel);

  	virtual void loop();

	virtual String html();
	virtual String buildRestReply(bool metric);

	virtual String name() { return String ("Flite Sensor"); };
  private:
	Adafruit_VL53L0X lox = Adafruit_VL53L0X();
	uint16_t distance;
#if XMA
	float ema = -1.0f;
	float alpha = 0.1f;
	uint16_t base_value = 0;
	uint16_t reset_threshold = 8;
#endif
	float m, b;
	float temperature = NAN;
	float temp_offset;
	float pressure = NAN;
	float press_offset;
};

#endif
