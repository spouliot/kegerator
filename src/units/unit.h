#ifndef __UNIT_H__
#define __UNIT_H__

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class Host;

class Unit {
	Host *host;
	uint8_t ch;
  public:
	Unit(Host *host, uint8_t channel);
	virtual ~Unit() {};

	uint8_t getChannel() { return ch; }
	String getUrl();
	
  	virtual void loop() = 0;

	virtual String html() { return String(""); };
	virtual String buildRestReply(bool metric) = 0;
	virtual void sentRestResponse(AsyncWebServerRequest *request);

	virtual String name() = 0;
};

class EmptyUnit : public Unit {
  public:
  	EmptyUnit();

  	virtual void loop() {};

	virtual String buildRestReply(bool metric) { return String(""); };
	virtual void sentRestResponse(AsyncWebServerRequest *request);

	virtual String name() { return String("empty"); };
};

#endif
