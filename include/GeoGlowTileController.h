#ifndef GEO_GLOW_TILE_CONTROLLER_H
#define GEO_GLOW_TILE_CONTROLLER_H

#include <Arduino.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <UUID.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include "MQTTClient.h"
#include "NanoleafApiWrapper.h"
#include "ColorPaletteAdapter.h"

// Function Prototypes
void initializeUUID();
void loadConfigFromFile();
void saveConfigToFile();
void generateMDNSNanoleafURL();
void attemptNanoleafConnection();
void setupMQTTClient();
void publishStatus();
void saveConfigCallback();
void connectToWiFi();
void promptForCredentials();

// Constants
extern const unsigned long PUBLISH_INTERVAL;
extern const char* CONFIG_FILE;
extern const size_t CONFIG_JSON_SIZE;
extern const int MDNS_RETRIES;
extern const int MDNS_RETRY_DELAY;

// WiFi Credentials
extern char wifiSSID[40];
extern char wifiPassword[40];

// Global Variables
extern WiFiClient wifiClient;
extern MQTTClient mqttClient;
extern NanoleafApiWrapper nanoleaf;
extern ColorPaletteAdapter colorPaletteAdapter;

extern unsigned long lastPublishTime;
extern char mqttBroker[40];
extern char mqttPort[6];
extern char nanoleafBaseUrl[55];
extern char nanoleafAuthToken[33];
extern char friendId[36];
extern char deviceId[36];

extern bool shouldSaveConfig;

#endif // GEO_GLOW_TILE_CONTROLLER_H
