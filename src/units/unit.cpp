#include "unit.h"
#include "host.h"

Unit::Unit (Host *host, uint8_t channel)
{
  this->host = host;
  ch = channel;
}

void Unit::sentRestResponse (AsyncWebServerRequest *request)
{
  bool metric = !request->hasParam("us");
  request->send(200, "text/json", buildRestReply (metric));
}

String Unit::getUrl()
{
  return "http://" + host->getName() + ".local./get?channel=" + String(ch);
}


EmptyUnit::EmptyUnit () : Unit (nullptr, -1)
{
}

void EmptyUnit::sentRestResponse (AsyncWebServerRequest *request)
{
  request->send (404, "text/plain", "Not found");
}
