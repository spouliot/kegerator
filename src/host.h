#ifndef __HOST_H__
#define __HOST_H__

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Preferences.h>

#include "ClosedCube_TCA9548A.h"

#include "units/unit.h"
#include "units/envii.h"
#include "units/flite.h"

// NOTE: provide your own with defines for DEFAULT_SSID and DEFAULT_PASSWORD
#include "private.h"

#define PaHub_I2C_ADDRESS	0x70

class Unit;
class EmptyUnit;

class Host {
	ClosedCube::Wired::TCA9548A hub;
	EmptyUnit empty; // shared
	Unit* units[6] = { &empty, &empty, &empty, &empty, &empty, &empty };

	AsyncWebServer server = AsyncWebServer(80);

	Preferences preferences;
	String ssid;
	String password;
	String name;

	bool isEmpty(int channel) { return units[channel] == &empty; };
	void plugAndPlay(int channel);
  public:
	Host() {};

	void setup();
	void begin();
	void scan();

	String getSsid() { return ssid; };
	String getPassword() { return password; }
	String getName() { return name; }
};

#endif
