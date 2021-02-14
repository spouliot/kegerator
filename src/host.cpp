#include "host.h"

void Host::setup()
{
  hub.address(PaHub_I2C_ADDRESS);

  preferences.begin("kegerator", /* readonly */ false);
  name = preferences.getString("name", "");
  if (name.length() == 0) {
    uint8_t mac[6];
    char buffer[14];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    sprintf(buffer, "kegtor-%02X%02X%02X", mac[3], mac[4], mac[5]);
    name = String(buffer);
  }
  ssid = preferences.getString("ssid", DEFAULT_SSID);
  password = preferences.getString("password", DEFAULT_PASSWORD);

  server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
    String html = "<html><body>";
    html += "<h1>Host</h1>";
    html += "<ul><li>SSID: " + getSsid() + "</li>";
    html += "<li>IP address: " + WiFi.localIP().toString() + " (DHCP)</li>";
    html += "<li>MAC address: " + WiFi.macAddress() + "</li>";
    html += "<li>MDNS name: " + getName() + "</li></ul>";
    for(uint8_t channel=0; channel<TCA9548A_MAX_CHANNELS; channel++) {
      html += "<h1>Channel #";
      html += channel;
      html += ": ";
      html += units[channel]->name();
      html += "</h1>\n<p>";
      html += units[channel]->html();
      html += "</p>\n";
    }
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/config", HTTP_GET, [this](AsyncWebServerRequest *request){
    String html = "<html><body>";
    html += "<h1>Host</h1>";
    html += "<form action=\"/updateWiFi\" method=\"POST\">";
    html += "<label for=\"ssid\">SSID: </label>";
    html += "<input type=\"text\" name=\"ssid\" placeholder=\"SSID\" value=\"" + getSsid() + "\">";
    html += "<label for=\"password\">Password: </label>";
    html += "<input type=\"password\" name=\"password\" placeholder=\"Password\" value=\"\">";
    html += "<button type=\"submit\">Update</button>";
    html += "</form>";
    // TODO: mdns name update
    for(uint8_t channel=0; channel<TCA9548A_MAX_CHANNELS; channel++) {
      html += "<h1>Channel #";
      html += channel;
      html += ": ";
      html += units[channel]->name();
      html += "</h1>\n<p>";
      // TODO: allow sensors configuration / calibration
      html += "</p>\n";
    }
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/get", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (request->hasParam("channel")) {
        int port = request->getParam("channel")->value().toInt();
        if (port >= 0 && port <= TCA9548A_MAX_CHANNELS) {
          units[port]->sentRestResponse(request);
          return;
        }
      request->send (404, "text/plain", "Not found");
    }
    // TODO return unit info + channel list
    request->send (404, "text/plain", "Not found");
  });

  // Send a POST request to <IP>/post with a form field message set to <message>
  server.on("/updateWiFi", HTTP_POST, [this](AsyncWebServerRequest *request){
      if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
        String new_ssid = request->getParam("ssid")->value();
        String new_pass = request->getParam("password")->value();
        preferences.putString("ssid", new_ssid);
        preferences.putString("password", new_pass);
        preferences.end();
        request->send(200, "text/plain", "Updated. Rebooting...");
        esp_restart();
      } else {
        request->send(400, "text/plain", "Bad Request");
      }
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });
}

void Host::begin()
{
  server.begin();
}

// detect which unit(s) are available on the I2C bus
void Host::plugAndPlay(int channel)
{
  bool is_empty = isEmpty(channel);

  // look for well known (supported) addresses

  Wire.beginTransmission(0x44);
  uint8_t returnCode = Wire.endTransmission();
  if (returnCode == 0) {
    if (is_empty)
      units[channel] = new EnvUnit (this, channel);
    return;
  }

  Wire.beginTransmission(0x29);
  returnCode = Wire.endTransmission();
  if (returnCode == 0) {
    if (is_empty)
      units[channel] = new FliteUnit (this, channel);
    return;
  }

  // if we cannot find any unit in this channel is empty
  if (!is_empty) {
    delete units [channel];
    units[channel] = &empty;
  }
}

void Host::scan()
{
  // let's assume there's an hub and scan each channel
  for (int8_t channel = 0; channel < TCA9548A_MAX_CHANNELS; channel++) {
    uint8_t returnCode = hub.selectChannel(channel);
#if DEBUG
    Serial.printf("CH%d: (%d)\n", channel, returnCode);
#endif
    // if there's no hub (2) then fall back to what's directly connected
    if (returnCode == 2)
      channel = 0;

    plugAndPlay(channel);
    units[channel]->loop();

    // stop scanning channels if there's no hub    
    if (returnCode == 2)
      return;
  }
}
