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

WiFiManager wifiManager;
WiFiClient wifiClient;
MQTTClient mqttClient(wifiClient);
NanoleafApiWrapper nanoleaf(wifiClient);

ColorPaletteAdapter colorPaletteAdapter(nanoleaf);

unsigned long lastPublishTime = 30000;

char mqttBroker[40];
char mqttPort[6] = "1883";
char nanoleafBaseUrl[55] = "";
char nanoleafAuthToken[33] = "";
char friendId[36] = "";
char deviceId[36] = "";

bool shouldSaveConfig = false;

void saveConfigCallback() {
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

void setup() {
    Serial.begin(115200);
    delay(10);

    UUID uuid;

    uint32_t seed1 = random(999999999);
    uint32_t seed2 = random(999999999);

    uuid.seed(seed1, seed2);
    uuid.generate();
    strcpy(deviceId, uuid.toCharArray());


    if (SPIFFS.begin()) {
        Serial.println("mounted file system");
        if (SPIFFS.exists("/config.json")) {
            Serial.println("reading config file");
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                Serial.println("opened config file");
                size_t size = configFile.size();
                std::unique_ptr<char[]> buf(new char[size]);
                configFile.readBytes(buf.get(), size);

                JsonDocument jsonConfig;
                deserializeJson(jsonConfig, buf.get());
                Serial.println("parsed json");
                strcpy(mqttBroker, jsonConfig["mqttBroker"]);
                strcpy(mqttPort, jsonConfig["mqttPort"]);
                strcpy(nanoleafBaseUrl, jsonConfig["nanoleafBaseUrl"]);
                strcpy(nanoleafAuthToken, jsonConfig["nanoleafAuthToken"]);
                strcpy(deviceId, jsonConfig["deviceId"]);
                strcpy(friendId, jsonConfig["friendId"]);
            } else {
                Serial.println("failed to load json config");
            }
            configFile.close();
        } else {
            if (MDNS.begin("esp8266") && MDNS.queryService("nanoleafapi", "tcp") > 0) {
                IPAddress ip = MDNS.IP(0);
                uint16_t port = MDNS.port(0);

                snprintf(
                    nanoleafBaseUrl,
                    sizeof(nanoleafBaseUrl),
                    "http://%u.%u.%u.%u:%u",
                    ip[0], ip[1], ip[2], ip[3], port
                );
            }
        }
    } else {
        Serial.println("failed to mount FS");
    }

    WiFiManagerParameter customMqttBroker("mqttBroker", "mqtt broker", mqttBroker, 40);
    WiFiManagerParameter customMqttPort("mqttPort", "mqtt port", mqttPort, 6);
    WiFiManagerParameter customNanoleafBaseUrl(
        "nanoleafBaseUrl",
        "http://<Nanoleaf-Base-Url[:<Port>]>",
        nanoleafBaseUrl,
        55
    );
    WiFiManagerParameter customFriendId("friendId", "<Friend-ID>", friendId, 36);

    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.addParameter(&customMqttBroker);
    wifiManager.addParameter(&customMqttPort);
    wifiManager.addParameter(&customNanoleafBaseUrl);
    wifiManager.addParameter(&customFriendId);

    if (!wifiManager.autoConnect("GeoGlow")) {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
        EspClass::restart();
        delay(5000);
    }

    Serial.println("connected...yeey :)");
    strcpy(mqttBroker, customMqttBroker.getValue());
    strcpy(mqttPort, customMqttPort.getValue());
    strcpy(nanoleafBaseUrl, customNanoleafBaseUrl.getValue());
    strcpy(friendId, customFriendId.getValue());

    friendId[sizeof(friendId) - 1] = '\0';
    deviceId[sizeof(deviceId) - 1] = '\0';

    Serial.println("The values in the file are: ");
    Serial.println("\tmqttBroker : \t\t" + String(mqttBroker));
    Serial.println("\tmqttPort : \t\t" + String(mqttPort));
    Serial.println("\tnanoleafBaseUrl : \t" + String(nanoleafBaseUrl));
    Serial.println("\tnanoleafAuthToken : \t" + String(nanoleafAuthToken));
    Serial.println("\tfriendId : \t\t" + String(friendId));
    Serial.println("\tdeviceId : \t\t" + String(deviceId));

    Serial.print("local ip: ");
    Serial.println(WiFi.localIP());

    nanoleaf.setup(nanoleafBaseUrl, nanoleafAuthToken);
    delay(1000);

    while (true) {
        Serial.print("Attempting Nanoleaf connection...");

        if (nanoleaf.isConnected()) {
            Serial.println("connected");
            break;
        }
        Serial.println("failed");

        if (String newToken = nanoleaf.generateToken(); newToken != "") {
            newToken.toCharArray(nanoleafAuthToken, sizeof nanoleafAuthToken);
            nanoleaf.setup(nanoleafBaseUrl, nanoleafAuthToken);
            shouldSaveConfig = true;
        }
        delay(5000);
    }

    if (shouldSaveConfig) {
        Serial.println("saving config");
        JsonDocument jsonConfig;
        jsonConfig["mqttBroker"] = mqttBroker;
        jsonConfig["mqttPort"] = mqttPort;
        jsonConfig["nanoleafBaseUrl"] = nanoleafBaseUrl;
        jsonConfig["nanoleafAuthToken"] = nanoleafAuthToken;
        jsonConfig["friendId"] = friendId;
        jsonConfig["deviceId"] = deviceId;

        File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile) {
            Serial.println("failed to open config file for writing");
        }
        serializeJson(jsonConfig, Serial);
        serializeJson(jsonConfig, configFile);
        configFile.close();
    }

    mqttClient.setup(mqttBroker, String(mqttPort).toInt(), friendId, deviceId);
    mqttClient.addTopicAdapter(&colorPaletteAdapter);

    nanoleaf.setPower(true);
    delay(1500);
    nanoleaf.setPower(false);
}

void loop() {
    mqttClient.loop();

    if (millis() - lastPublishTime >= 30000) {
        JsonDocument jsonPayload;
        jsonPayload["friendId"] = friendId;
        jsonPayload["deviceId"] = deviceId;
        jsonPayload["panelIds"] = JsonArray();

        for (const String &panelId: nanoleaf.getPanelIds()) {
            jsonPayload["panelIds"].add(panelId);
        }

        mqttClient.publish("GeoGlow/Friend-Service/ping", jsonPayload);

        lastPublishTime = millis();
    }
}
