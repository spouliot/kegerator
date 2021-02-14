#include "envii.h"

EnvUnit::EnvUnit(Host *host, uint8_t channel) : Unit(host, channel)
{
  if (!sht.begin(SHT30_ADDR)) {
#if DEBUG
    Serial.println ("SHT30 init failed");
#endif
  } else {
#if DEBUG
    uint16_t stat = sht.readStatus();
    Serial.print(stat, HEX);
    Serial.println();
#endif
  }

  if (!bmp.begin(BMP280_ADDR)) {
#if DEBUG
    Serial.println ("BMP280 init failed");
#endif
    return;
  }

  bmp_temp = bmp.getTemperatureSensor();
  bmp_pressure = bmp.getPressureSensor();

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
#if DEBUG
  bmp_temp->printSensorDetails();
  bmp_pressure->printSensorDetails();
#endif
}

void EnvUnit::loop()
{
  sht.read();
  temperature = sht.getTemperature();
  humidity = sht.getHumidity();
#if DEBUG
  Serial.printf("SHT30\t%.1f C\t%.1f %%\n", temperature, humidity);
#endif
  if (!bmp_temp || !bmp_pressure)
    return;

  sensors_event_t temp_event, pressure_event;
  bmp_temp->getEvent(&temp_event);
  bmp_pressure->getEvent(&pressure_event);
  
#if DEBUG
  Serial.printf("BMP280\t%.1f C\t%.1f hPa\n", temp_event.temperature, pressure_event.pressure);
#endif
  pressure = pressure_event.pressure / 10.0f; // to kPa
  
  // average temperature between both sensors (between .2 and .4 C difference)
  temperature += temp_event.temperature;
  temperature /= 2.0f;
}

String EnvUnit::html ()
{
  String fragment = "<ul><li>Temperature: ";
  fragment += String(temperature, 1);
  fragment += "&deg;C</li>";
  fragment += "<li>Humidity: ";
  fragment += String(humidity, 1);
  fragment += " %</li>";
  fragment += "<li>Pressure: ";
  fragment += String(pressure, 1);
  fragment += " kPa</li></ul>";
  fragment += "<p>Sample query</p>";
  fragment += "<code><a href=\"" + getUrl() + "\">" + getUrl() + "</a><a href=\"" + getUrl() + "&us\">[&amp;us]</a></code>";
  fragment += "<p>REST response</p>";
  fragment += "<code>" + buildRestReply(true) + "</code>";
  return fragment;
}

String EnvUnit::buildRestReply(bool metric)
{
  float t = temperature;
  String t_unit = "DegC";
  float p = pressure;
  String p_unit = "kPa";
  String h_unit = "%";
  if (!metric) {
    // celcius to fahrenheit
    t = (temperature * 9.0f / 5.0f) + 32.0f;
    t_unit = "DegF";
    // kPa to PSI pressure
    p = pressure * 0.145037738;
    p_unit = "PSI";
  }

  String reply = "{ \"temperature\": \"";
  reply += String(t, 1);
  reply += "\", \"temperatureUnits\": \"" + t_unit;
  reply += "\", \"humidity\": \"";
  reply += String(humidity, 1);
  reply += "\", \"humidityUnits\": \"" + h_unit;
  reply += "\", \"pressure\": \"";
  reply += String(p, 1);
  reply += "\", \"pressureUnits\": \"" + p_unit;
  reply += "\" }";
  return reply;
}
