#include "Controller.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

// Constants
const unsigned long PUBLISH_INTERVAL = 30000;
const char* CONFIG_FILE = "/config.json";
const size_t CONFIG_JSON_SIZE = 1024;
const int MDNS_RETRIES = 10;      // Number of times to retry mDNS query
const int MDNS_RETRY_DELAY = 2000;// Delay between retries in milliseconds
const char* DEFAULT_MQTT_BROKER = "hivemq.dock.moxd.io";
const int DEFAULT_MQTT_PORT = 1883;

// Global Variables
WiFiManager wifiManager;
WiFiClient wifiClient;
MQTTClient mqttClient(wifiClient);
NanoleafApiWrapper nanoleaf(wifiClient);
ColorPaletteAdapter colorPaletteAdapter(nanoleaf);

unsigned long lastPublishTime = 0;

char ssid[32];
char password[64];
char nanoleafBaseUrl[55] = "";
char nanoleafAuthToken[33] = "";
char friendId[45] = ""; // Max Length: 36 (name) + 1 (seperator) + 8 (short UUID)
char name[36] = "";
char groupId[36] = "";

bool shouldSaveConfig = false;

// Function Definitions
void saveConfigCallback() {
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

void generateShortUUID(char* uuid, size_t length) {
    const char* hexChars = "0123456789ABCDEF";
    for (size_t i = 0; i < length - 1; ++i) {
        uuid[i] = hexChars[random(16)];
    }
    uuid[length - 1] = '\0';
}

void initializeUUID() {
    char shortUUID[9];
    generateShortUUID(shortUUID, sizeof(shortUUID));
    snprintf(friendId, sizeof(friendId), "%s@%s", name, shortUUID);
}

void generateMDNSNanoleafURL() {
    if (MDNS.begin("esp8266")) {
        Serial.println("MDNS responder started");

        int retryCount = 0;
        while (retryCount < MDNS_RETRIES) {
            Serial.printf("Attempt %d to query Nanoleaf service...\n", retryCount + 1);
            // Search for the Nanoleaf service (_nanoleafapi._tcp.local)
            int n = MDNS.queryService("nanoleafapi", "tcp");

            if (n > 0) {
                // Ideally, we expect just one Nanoleaf device to respond
                String ip = MDNS.IP(0).toString();
                int port = MDNS.port(0);

                snprintf(nanoleafBaseUrl, sizeof(nanoleafBaseUrl), "http://%s:%d", ip.c_str(), port);
                Serial.printf("Nanoleaf service found at: %s\n", nanoleafBaseUrl);
                shouldSaveConfig = true;
                saveConfigToFile();
                return;
            } else {
                Serial.println("No Nanoleaf service found, retrying...");
                retryCount++;
                delay(MDNS_RETRY_DELAY);
            }
        }

        // If we exit the loop without finding a service
        Serial.println("Failed to discover Nanoleaf service after retries");
    } else {
        Serial.println("Error starting mDNS");
    }
}

void setup() {
    Serial.begin(115200);
    delay(10);

    initializeUUID();
    loadConfigFromFile();

    if (strlen(ssid) == 0 || strlen(password) == 0) {
        setupWiFiManager();
    } else {
        connectToWifi();
    }

    if (strlen(name) != 0 && strlen(friendId) == 0) {
        initializeUUID();
    }

    if (strlen(nanoleafBaseUrl) == 0) {
        generateMDNSNanoleafURL();
    }
    
    setupMQTTClient();
    attemptNanoleafConnection();

    if (shouldSaveConfig) {
        saveConfigToFile();
    }
}

void loop() {
    mqttClient.loop();

    if (millis() - lastPublishTime >= PUBLISH_INTERVAL) {
        publishStatus();
        lastPublishTime = millis();
    }
}

void loadConfigFromFile() {
    if (!SPIFFS.begin()) {
        Serial.println("Failed to mount FS");
        return;
    }
    
    if (!SPIFFS.exists(CONFIG_FILE)) {
        return;
    }

    File configFile = SPIFFS.open(CONFIG_FILE, "r");
    Serial.println("Reading config file");
    
    size_t size = configFile.size();
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);

    StaticJsonDocument<CONFIG_JSON_SIZE> jsonConfig;
    DeserializationError error = deserializeJson(jsonConfig, buf.get());

    if (error) {
        Serial.println("Failed to parse JSON config file");
        return;
    }

    strcpy(ssid, jsonConfig["ssid"]);
    strcpy(password, jsonConfig["password"]);
    strcpy(nanoleafAuthToken, jsonConfig["nanoleafAuthToken"]);
    strcpy(name, jsonConfig["name"]);
    strcpy(groupId, jsonConfig["groupId"]);
    strcpy(nanoleafBaseUrl, jsonConfig["nanoleafBaseUrl"]);
    strcpy(friendId, jsonConfig["friendId"]);

    configFile.close();
    Serial.println("Parsed JSON config");
}

void saveConfigToFile() {
    if (!shouldSaveConfig) return; // No need to save if no changes

    SPIFFS.begin();
    File configFile = SPIFFS.open(CONFIG_FILE, "w");
    
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return;
    }

    StaticJsonDocument<CONFIG_JSON_SIZE> jsonConfig;
    jsonConfig["ssid"] = ssid;
    jsonConfig["password"] = password;
    jsonConfig["nanoleafAuthToken"] = nanoleafAuthToken;
    jsonConfig["friendId"] = friendId;
    jsonConfig["name"] = name;
    jsonConfig["nanoleafBaseUrl"] = nanoleafBaseUrl;
    jsonConfig["groupId"] = groupId;

    serializeJson(jsonConfig, configFile);
    configFile.close();
    Serial.println("Config saved successfully");
}

void attemptNanoleafConnection() {
    nanoleaf.setup(nanoleafBaseUrl, nanoleafAuthToken);
    int attempts = 0;
    const int maxAttempts = 5;

    while (!nanoleaf.isConnected() && attempts < maxAttempts) {
        Serial.printf("Attempting Nanoleaf connection... (%d/%d)\n", attempts + 1, maxAttempts);
        delay(5000);

        if (!nanoleaf.isConnected()) {
            String newToken = nanoleaf.generateToken();
            if (!newToken.isEmpty()) {
                newToken.toCharArray(nanoleafAuthToken, sizeof(nanoleafAuthToken));
                nanoleaf.setup(nanoleafBaseUrl, nanoleafAuthToken);
                shouldSaveConfig = true;
            }
        }
        attempts++;
    }

    if (nanoleaf.isConnected()) {
        Serial.println("Nanoleaf connected");
    } else {
        Serial.println("Failed to connect to Nanoleaf with saved baseURL, reattempting MDNS lookup.");
        generateMDNSNanoleafURL();
        attemptNanoleafConnection();
    }
}

void connectToWifi() {
    Serial.println("Connecting to Wi-Fi using saved credentials");

    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConneted to Wi-Fi");
    } else {
        Serial.println("\nFailed to connect to Wi-Fi");
        setupWiFiManager();
    }
}

void setupWiFiManager() {
    WiFiManager wifiManager;

    wifiManager.setSaveConfigCallback(saveConfigCallback);

    WiFiManagerParameter customGroupId("groupId", "Group ID", groupId, 36);
    WiFiManagerParameter customName("name", "Name", name, 36);

    wifiManager.addParameter(&customGroupId);
    wifiManager.addParameter(&customName);

    if (!wifiManager.autoConnect("GeoGlow")) {
        Serial.println("Failed to connect and hit timeout");
        delay(3000);
        ESP.restart();
        delay(5000);
    }

    Serial.println("Connected");

    strncpy(ssid, WiFi.SSID().c_str(), sizeof(ssid) - 1);
    strncpy(password, WiFi.psk().c_str(), sizeof(password) - 1);
    strncpy(groupId, customGroupId.getValue(), sizeof(groupId) - 1);
    strncpy(name, customName.getValue(), sizeof(name) - 1);

    shouldSaveConfig = true;

    if (strlen(groupId) >= sizeof(groupId) - 1) groupId[sizeof(groupId) - 1] = '\0';
    if (strlen(name) >= sizeof(name) - 1) name[sizeof(name) - 1] = '\0';

    initializeUUID();
}

void setupMQTTClient() {
    mqttClient.setup(DEFAULT_MQTT_BROKER, DEFAULT_MQTT_PORT, friendId);
    mqttClient.addTopicAdapter(&colorPaletteAdapter);

    nanoleaf.setPower(true);
    delay(1500);
    nanoleaf.setPower(false);
}

void publishStatus() {
    StaticJsonDocument<200> jsonPayload;
    jsonPayload["friendId"] = friendId;
    JsonArray panelIds = jsonPayload.createNestedArray("panelIds");

    for (const String &panelId: nanoleaf.getPanelIds()) {
        panelIds.add(panelId);
    }

    mqttClient.publish("GeoGlow/Friend-Service/ping", jsonPayload);
}