#ifndef __ENVII_UNIT__H__
#define __ENVII_UNIT__H__

#include <SHT31.h>
#include <Adafruit_BMP280.h>

#include "unit.h"

// https://m5stack.com/products/env-ii-unit

#define SHT30_ADDR	0x44
#define BMP280_ADDR	0x76

class EnvUnit : public Unit {
  public:
  	EnvUnit(Host *host, uint8_t channel);

  	virtual void loop();

	virtual String html();
	virtual String buildRestReply(bool metric);

	virtual String name() { return String ("ENV II Sensor"); };
  private:
	// SHT30 (0x44)
	// https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/unit/SHT3x_Datasheet_digital.pdf
	SHT31 sht;
	float temperature;
	float humidity;
	// BMP280 (0x76)
	// https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/hat/BMP280-DS001-11_en.pdf
	Adafruit_BMP280 bmp;
	Adafruit_Sensor *bmp_temp;
	Adafruit_Sensor *bmp_pressure;
	float pressure;
};

#endif
