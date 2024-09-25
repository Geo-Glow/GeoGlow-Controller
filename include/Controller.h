#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#else
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#endif

#include <DNSServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include "MQTTClient.h"
#include "NanoleafApiWrapper.h"
#include "ColorPaletteAdapter.h"

// Function Prototypes
void initializeUUID();
void loadConfigFromFile();
void saveConfigToFile();
bool generateMDNSNanoleafURL();
void attemptNanoleafConnection();
void setupWiFiManager();
void setupMQTTClient();
void publishStatus();
void saveConfigCallback();
void connectToWifi();
void generateShortUUID(char *uuid, size_t length);
void connectToWifi(bool useSavedCredentials = true);
void registerNanoleafEvents();
#endif // CONTROLLER_H