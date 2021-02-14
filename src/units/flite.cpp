#include "flite.h"

#define DEBUG 0

FliteUnit::FliteUnit (Host *host, uint8_t channel) : Unit (host, channel)
{
  String name = "channel#" + String (channel);

  Preferences preferences;
  preferences.begin(name.c_str(), /* readonly */ false);
  float cal_high = preferences.getFloat("cal_high", 4.0f);
  float cal_low = preferences.getFloat("cal_low", 0.0f);
  float dist_high = preferences.getFloat("dist_high", 65.0f);
  float dist_low = preferences.getFloat("dist_low", 575.0f);
  m = (cal_high - cal_low) / (dist_high - dist_low);
  b = cal_high - (m * dist_high);

  // kegerator is 2-3C colder than the liquid inside the keg ??? needs investigation
  temp_offset = preferences.getFloat("temp_offset", 0.0f);

  // this is pretty much the ambiant pressure that the (optional) ENVII sensor reports
  press_offset = preferences.getFloat("press_offset", 14.6f);

  if (!lox.begin(VL53L0X_I2C_ADDR, DEBUG, &Wire, Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY)) {
    Serial.println("Failed to boot VL53L0X");
  }
}

void FliteUnit::loop ()
{
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, DEBUG);

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    distance = measure.RangeMilliMeter;
#if XMA
 		// https://en.wikipedia.org/wiki/Moving_average#Exponential_moving_average
		if ((ema == -1.0f) || (ema < base_value - reset_threshold) || (ema > base_value + reset_threshold)) {
      // Detect large change (like 12oz pour) and reset
      // e.g. 500 -> 5G / 19L -> 1 beer 12oz/355ml == 53.5 beers, 9.25 mm / beer
      ema = distance;
      base_value = distance;
    } else
      ema = alpha * distance + (1.0f - alpha) * ema;
#endif

#if DEBUG
#if XMA
    float g = ((m * distance) + b);
    float l = g * 3.78541f;
    float ema_g = ((m * ema) + b);
    float ema_l = (ema_g * 3.78541f);
    Serial.printf("CH#%d  /  %d mm  %.1f G  %.1f L  /  ema: %f mm  %.1f G  %.1f L\n", getChannel(), distance, g, l, ema, ema_g, ema_l);
#else
    Serial.printf("%d\n", distance);
#endif // XMA
#endif // DEBUG
  }

  // in case the temp/pressure sensor is not present, either by choice
  // or testing code with a https://docs.m5stack.com/#/en/unit/tof
  if (Wire.requestFrom((uint8_t) 0x48, (uint8_t) 4) != 4)
    return;

  uint8_t b1 = Wire.read();
  uint8_t b2 = Wire.read();
  uint8_t b3 = Wire.read();
  uint8_t b4 = Wire.read();

  // https://sensing.honeywell.com/spi-comms-digital-ouptu-pressure-sensors-tn-008202-3-en-final-30may12.pdf

  uint8_t status = (b1 & 0xc0) >> 6; // 2 bits
  if (status != 0)
    return;

  uint16_t pcount = ((b1 & 0x3f) << 8) | b2; // 14 bits
  uint16_t tcount = ((b3 << 8) | (b4 & 0xe0)) >> 5; // 1 bits

  const uint16_t oMin = 0x666;
  const uint16_t oMax = 0x3999;
  const float pMin = 0.0f;
  const float pMax = 150.0f;
  pressure = ((float) ((pcount - oMin) * (pMax - pMin))) / (oMax - oMin) + pMin;
  pressure -= press_offset;

  temperature = ((tcount / 2047.0f) * 200.0f) - 50.0f; // in Celcius
  temperature -= temp_offset;
#if DEBUG
  Serial.printf ("HSC status %d   pressure count %d value %.1f   temperature count %d value %.1f\n", status, pcount, pressure, tcount, temperature);
#endif
}

String FliteUnit::html ()
{
  String fragment = "<ul><li>Volume: ";
  fragment += String((m * distance) + b, 1);
  fragment += " gallons</li>\n<li>Temperature: ";
  fragment += String(temperature, 1);
  fragment += "&deg;C</li>\n<li>Pressure: ";
  fragment += String(pressure, 1);
  fragment += " psi</li></ul>\n";
  fragment += "<p>Sample query</p>";
  fragment += "<code><a href=\"" + getUrl() + "\">" + getUrl() + "</a><a href=\"" + getUrl() + "&us\">[&amp;us]</a></code>";
  fragment += "<p>REST response</p>";
  fragment += "<code>" + buildRestReply(true) + "</code>";
  return fragment;
}

String FliteUnit::buildRestReply (bool metric)
{
#if XMA
  float l = (m * ema) + b;
#else
  float l = (m * distance) + b;
#endif
  String l_unit = "Gallons";
  float t = temperature;
  String t_unit = "DegC";
  float p = pressure;
  String p_unit = "PSI";
  if (metric) {
    // US Liquid Gallons to litres
	  l *= 3.78541f;
    l_unit = "Litres";
    // PSI -> kPa
	p *= 6.89476f;
    p_unit = "kPa";
  } else {
    t = (temperature * 9.0f / 5.0f) + 32.0f;
    t_unit = "DegF";
  }

  String reply = "{ \"level\": \"";
  reply += String(l, 1);
  reply += "\", \"levelUnits\": \"" + l_unit;
  reply += "\", \"temperature\": \"";
  reply += String(t, 1);
  reply += "\", \"temperatureUnits\": \"" + t_unit;
  reply += "\", \"pressure\": \"";
  reply += String(p, 1);
  reply += "\", \"pressureUnits\": \"" + p_unit;
  reply += "\" }";
  return reply;
}
