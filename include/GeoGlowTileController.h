#ifndef GEO_GLOW_TILE_CONTROLLER_H
#define GEO_GLOW_TILE_CONTROLLER_H

#include <Arduino.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <UUID.h>
#include <ESP8266mDNS.h>
#include "MQTTClient.h"
#include "NanoleafApiWrapper.h"
#include "ColorPaletteAdapter.h"

// Function Prototypes
void initializeUUID();
void loadConfigFromFile();
void saveConfigToFile();
void generateMDNSNanoleafURL();
void attemptNanoleafConnection();
void setupWiFiManager();
void setupMQTTClient();
void publishStatus();
void saveConfigCallback();

#endif // GEO_GLOW_TILE_CONTROLLER_H