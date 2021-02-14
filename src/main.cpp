#include <Arduino.h>
#include "M5Atom.h"

#include <ESPmDNS.h>
#include <WiFi.h>
#include <Wire.h>
#include <SimpleTimer.h>

#include "host.h"

Host host;
SimpleTimer timer;
bool led_on = true;

void scan()
{
  // blink blue LED while scanning
  led_on = !led_on;
  M5.dis.drawpix(0, led_on ? 0x0000FF : 0x000000);

  host.scan();
}

void setup()
{
  // M5ATOM: bool SerialEnable = true, bool I2CEnable = true, bool DisplayEnable = false
  // but the default I2C is not on the grove (but on the pins) so we enable it manually
  M5.begin(true, false, true);
  Wire.begin(26,32,10000);
  // set LED to RED until we know things, like WiFi, works as expected
  M5.dis.drawpix(0, 0x00FF00);

  host.setup();

  WiFi.mode(WIFI_STA);
  WiFi.begin(host.getSsid().c_str(), host.getPassword().c_str());
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    timer.setInterval(5000, esp_restart);
    return;
  }

  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  String name = host.getName();
  if (!MDNS.begin(name.c_str())) {
    Serial.println("Error starting mDNS");
    timer.setInterval(5000, esp_restart);
    return;
  }
  MDNS.addService("http", "tcp", 80);

  Serial.print("MDNS: ");
  Serial.println(name);

  host.begin();
  // blink LED while asking the host to scan all sensors every second
  timer.setInterval(1000, scan);
}

void loop()
{
  M5.update();
  timer.run();
}
