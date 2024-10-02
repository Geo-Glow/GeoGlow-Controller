#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#else
#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#endif

#include <LittleFS.h>
#include "Controller.h"
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <SeeedOLED.h>

// Constants
const unsigned long PUBLISH_INTERVAL = 30000;
const char *CONFIG_FILE = "/config.json";
const size_t CONFIG_JSON_SIZE = 1024;
const char *API_URL_PREFIX = "http://139.6.56.197/friends/";

bool layoutChanged = false;
bool initialSetupDone = false;

// Global Variables
WiFiManager wifiManager;
WiFiClient wifiClientForMQTT;
WiFiClient wifiClientForHTTP;
HTTPClient httpClient;
MQTTClient mqttClient(wifiClientForMQTT);
NanoleafApiWrapper nanoleaf(wifiClientForMQTT);
ColorPaletteAdapter colorPaletteAdapter(nanoleaf);

// WIFI Constants
const int WIFI_MAX_ATTEMPTS = 10;      // Maximum Wifi connection attempts
const int WIFI_RETRY_DELAY = 500;      // Delay between each attempt
const int WIFI_CONNECT_TIMEOUT = 5000; // Maximum time to wait for the connection (in ms)

// WIFI Vars
char ssid[32];     // Wifi SSID
char password[64]; // Wifi Password

// MDNS Constants
const int MDNS_MAX_RETRIES = 10;           // Maximum MDNS Retries
const int MDNS_INITIAL_RETRY_DELAY = 1000; // Initial delay (in ms)
const float MDNS_BACKOFF_FACTOR = 1.5;     // Backoff factor for exponential delay

// MQTT Constants
const char *DEFAULT_MQTT_BROKER = "hivemq.dock.moxd.io"; // MQTT Broker Address
const int DEFAULT_MQTT_PORT = 1883;                      // MQTT Broker Port

// NanoLeaf Vars
char nanoleafBaseUrl[55] = "";   // NanoLeaf Baseurl (http://<ip>:<port>)
char nanoleafAuthToken[33] = ""; // Nanoleaf Auth Token

// Setup Vars
char friendId[45] = "";
char name[36] = "";
char groupId[36] = "";

unsigned long lastPublishTime = 0;
bool shouldSaveConfig = false;

void connectToWifi(bool useSavedCredentials)
{
    if (!useSavedCredentials || strlen(ssid) == 0 || strlen(password) == 0)
    {
        setupWiFiManager();
        return;
    }

    Serial.println("Connecting to Wi-Fi using saved credentials...");
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();
    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime) < WIFI_CONNECT_TIMEOUT && attempts < WIFI_MAX_ATTEMPTS)
    {
        delay(WIFI_RETRY_DELAY);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nConnected to Wi-Fi");
    }
    else
    {
        Serial.println("\nFailed to connect to Wi-Fi using saved credentials. Launching WiFi Manager...");
        setupWiFiManager();
    }
}

void setupWiFiManager()
{
    wifiManager.setDebugOutput(false); // Disable the debug output to keep Serial clean
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    WiFiManagerParameter customGroupId("groupId", "Group ID", groupId, 36);
    WiFiManagerParameter customName("name", "Name", name, 36);

    wifiManager.addParameter(&customGroupId);
    wifiManager.addParameter(&customName);

    if (!wifiManager.autoConnect("GeoGlow"))
    {
        Serial.println("Failed to connect via WiFi Manager and hit timeout");
        delay(3000);
        ESP.restart();
    }

    Serial.println("WiFi Manager has established a connection.");

    strncpy(ssid, WiFi.SSID().c_str(), sizeof(ssid) - 1);
    strncpy(password, WiFi.psk().c_str(), sizeof(password) - 1);
    strncpy(groupId, customGroupId.getValue(), sizeof(groupId) - 1);
    strncpy(name, customName.getValue(), sizeof(name) - 1);

    // Checks to prevent buffer overflow
    if (strlen(groupId) >= sizeof(groupId) - 1)
        groupId[sizeof(groupId) - 1] = '\0';
    if (strlen(name) >= sizeof(name) - 1)
        name[sizeof(name) - 1] = '\0';
}

// Function Definitions
void saveConfigCallback()
{
    Serial.println("Config should be saved.");
    shouldSaveConfig = true;
}

void generateShortUUID(char *uuid, size_t length)
{
    const char *hexChars = "0123456789ABCDEF";
    for (size_t i = 0; i < length - 1; ++i)
    {
        uuid[i] = hexChars[random(16)];
    }
    uuid[length - 1] = '\0';
}

void initializeUUID()
{
    char shortUUID[9];
    generateShortUUID(shortUUID, sizeof(shortUUID));
    snprintf(friendId, sizeof(friendId), "%s@%s", name, shortUUID);
    Serial.println("Initialized UUID.");
}

bool generateMDNSNanoleafURL()
{
    if (MDNS.begin("esp8266"))
    {
        Serial.println("MDNS wurde gestartet.");

        int retryCount = 0;
        int retryDelay = MDNS_INITIAL_RETRY_DELAY;

        while (retryCount < MDNS_MAX_RETRIES)
        {
            Serial.printf("Versuch %d den Nanoleaf Service zu finden...\n", retryCount + 1);
            int n = MDNS.queryService("nanoleafapi", "tcp");

            if (n > 0)
            {
                String ip = MDNS.IP(0).toString();
                int port = MDNS.port(0);

                snprintf(nanoleafBaseUrl, sizeof(nanoleafBaseUrl), "http://%s:%d", ip.c_str(), port);
                Serial.printf("Nanoleaf Service wurde gefunden: %s\n", nanoleafBaseUrl);
                saveConfigToFile();
                return true;
            }
            else
            {
                Serial.println("Es wurde kein Nanoleaf Service gefunden. Neuer Versuch...");
                retryCount++;
                delay(retryDelay);

                // Exponential Backoff
                retryDelay = static_cast<int>(retryDelay * MDNS_BACKOFF_FACTOR);
                if (retryDelay > MDNS_INITIAL_RETRY_DELAY * 10)
                {
                    retryDelay = MDNS_INITIAL_RETRY_DELAY * 10; // Cap the delay to a max of a factor of 10
                }
            }
        }

        // If we exit the loop without finding a service
        Serial.println("Es konnte kein Nanoleaf Service nach der maximalen Anzahl an Versuchen gefunden werden.");
    }
    else
    {
        Serial.println("Fehler beim Starten von MNDS.");
    }
    return false;
}

void loadConfigFromFile()
{
    if (!LittleFS.begin())
    {
        Serial.println("Failed to mount FS");
        return;
    }

    if (!LittleFS.exists(CONFIG_FILE))
    {
        Serial.println("Config file does not exist");
        LittleFS.end();
        return;
    }

    File configFile = LittleFS.open(CONFIG_FILE, "r");
    if (!configFile)
    {
        Serial.println("Failed to open config file");
        LittleFS.end();
        return;
    }

    size_t size = configFile.size();
    if (size > CONFIG_JSON_SIZE)
    {
        Serial.println("Config file is too large");
        configFile.close();
        LittleFS.end();
        return;
    }

    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);
    configFile.close();
    LittleFS.end();

    StaticJsonDocument<CONFIG_JSON_SIZE> jsonConfig;
    DeserializationError error = deserializeJson(jsonConfig, buf.get());

    if (error)
    {
        Serial.println("Failed to parse JSON config file");
        return;
    }

    strncpy(ssid, jsonConfig["ssid"], sizeof(ssid) - 1);
    strncpy(password, jsonConfig["password"], sizeof(password) - 1);
    strncpy(nanoleafAuthToken, jsonConfig["nanoleafAuthToken"], sizeof(nanoleafAuthToken) - 1);
    strncpy(name, jsonConfig["name"], sizeof(name) - 1);
    strncpy(groupId, jsonConfig["groupId"], sizeof(groupId) - 1);
    strncpy(nanoleafBaseUrl, jsonConfig["nanoleafBaseUrl"], sizeof(nanoleafBaseUrl) - 1);
    strncpy(friendId, jsonConfig["friendId"], sizeof(friendId) - 1);
    initialSetupDone = jsonConfig["setupDone"];

    Serial.println("Parsed JSON config");
}

void saveConfigToFile()
{
    if (!LittleFS.begin())
    {
        Serial.println("Failed to mount FS for save");
        return;
    }

    File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (!configFile)
    {
        Serial.println("Failed to open config file for writing");
        LittleFS.end();
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
    jsonConfig["setupDone"] = initialSetupDone;

    if (serializeJson(jsonConfig, configFile) == 0)
    {
        Serial.println("Failed to write JSON to config file");
    }
    else
    {
        Serial.println("Config saved successfully");
    }

    configFile.close();
    LittleFS.end();

    shouldSaveConfig = false;
}

void attemptNanoleafConnection()
{
    nanoleaf.setup(nanoleafBaseUrl, nanoleafAuthToken);
    int attempts = 0;
    const int maxAttempts = 5;

    while (!nanoleaf.isConnected() && attempts < maxAttempts)
    {
        Serial.printf("Attempting Nanoleaf connection... (%d/%d)\n", attempts + 1, maxAttempts);
        delay(6000);

        if (!nanoleaf.isConnected())
        {
            String newToken = nanoleaf.generateToken();
            if (!newToken.isEmpty())
            {
                newToken.toCharArray(nanoleafAuthToken, sizeof(nanoleafAuthToken));
                nanoleaf.setup(nanoleafBaseUrl, nanoleafAuthToken);
                shouldSaveConfig = true;
            }
        }
        attempts++;
    }

    if (nanoleaf.isConnected())
    {
        Serial.println("Nanoleaf connected");
        registerNanoleafEvents();
    }
    else
    {
        Serial.println("Failed to connect to Nanoleaf with saved baseURL, reattempting MDNS lookup.");
        generateMDNSNanoleafURL();
        attemptNanoleafConnection();
    }
}

void setupMQTTClient()
{
    mqttClient.setup(DEFAULT_MQTT_BROKER, DEFAULT_MQTT_PORT, friendId);
    mqttClient.addTopicAdapter(&colorPaletteAdapter);
}

void publishHeartbeat()
{
    if (!nanoleaf.isConnected())
    {
        Serial.println("Lost connection to nanoleafs. Trying to reconnect.");
        attemptNanoleafConnection();

        if (nanoleaf.isConnected())
        {
            Serial.println("Reconnecting worked! Continuing as before.");
        }
        else
        {
            Serial.println("Connection failed. Restarting ESP");
            ESP.restart();
        }
    }

    String url = String(API_URL_PREFIX) + friendId + "/heartbeat";
    httpClient.begin(wifiClientForHTTP, url);

    int httpResponseCode = httpClient.POST("{}");
    String responseMsg = httpClient.errorToString(httpResponseCode).c_str();
    switch (httpResponseCode)
    {
    case 204:
        Serial.println("Heartbeat posted");
        break;
    case 404:
        publishStatus();
        break;
    case 500:
        Serial.printf("Failed posting heartbeat: %s\n", responseMsg.c_str());
        break;
    default:
        Serial.printf("Unknown error occured: %s\n", responseMsg.c_str());
        break;
    }
    httpClient.end();
}

void publishStatus()
{
    StaticJsonDocument<200> jsonPayload;
    jsonPayload["friendId"] = friendId;
    jsonPayload["name"] = name;
    jsonPayload["groupId"] = groupId;
    JsonArray tileIds = jsonPayload.createNestedArray("tileIds");

    for (const String &panelId : nanoleaf.getPanelIds())
    {
        tileIds.add(panelId);
    }

    char buffer[512];
    size_t n = serializeJson(jsonPayload, buffer);

    String url = String(API_URL_PREFIX) + friendId;
    httpClient.begin(wifiClientForHTTP, url);
    httpClient.addHeader("Content-Type", "application/json");

    int httpResponseCode = httpClient.sendRequest("PATCH", (uint8_t *)buffer, n);

    if (httpResponseCode == 201 || httpResponseCode == 204)
    {
        Serial.printf("PATCH successfull, response code: %d\n", httpResponseCode);
    }
    else
    {
        Serial.printf("Error occured while making PATCH request: %s\n", httpClient.errorToString(httpResponseCode).c_str());
    }

    httpClient.end();
}

bool ensureNanoleafURL()
{
    if (strlen(nanoleafBaseUrl) == 0)
    {
        bool success = generateMDNSNanoleafURL();
        if (success)
        {
            Serial.println("NanoLeaf URL wurde gefunden.");
            return true;
        }
        else
        {
            Serial.println("Nanoleaf URL konnte nicht gefunden werden.");
            return false;
        }
    }
    else
        return true;
}

void registerNanoleafEvents()
{
    std::vector<int> eventIds = {2};
    bool success = false;
    const int maxRetries = 5;

    for (int attempt = 1; attempt <= maxRetries; attempt++)
    {
        success = nanoleaf.registerEvents(eventIds);
        if (success)
        {
            nanoleaf.setLayoutChangeCallback([]()
                                             { layoutChanged = true; });
            break;
        }
        else
        {
            Serial.printf("Event registration failed, attempt %d/%d\n", attempt, maxRetries);
            delay(100);
        }
    }

    if (!success)
    {
        Serial.println("Event registration failed after maximum retries. Restarting ESP.");
        ESP.restart();
    }

    Serial.println("Nanoleaf events registered.");
}

void publishInitialHeartbeat()
{
    publishHeartbeat();
    Serial.println("Initial Heartbeat published.");

    if (shouldSaveConfig)
    {
        saveConfigToFile();
        Serial.println("Configuration saved.");
    }
}

void userPrompt()
{
    Serial.println("Bitte geben Sie 'y' ein um fortzufahren.");

    while (true)
    {
        if (Serial.available() > 0)
        {
            char input = Serial.read();

            if (input == 'y' || input == 'Y')
            {
                Serial.println("Vielen Dank. Das Setup wird nun fortgesetzt...");
                break;
            }
            else
            {
                Serial.println("Ungültige Eingabe. Bitte geben Sie 'y' ein um fortzufahren.");
            }
        }
        delay(500);
    }
}

void initOled()
{
    Wire.begin();
    SeeedOled.init();
    SeeedOled.clearDisplay();
    SeeedOled.setNormalDisplay();
    SeeedOled.setPageMode();
}

void printToOled(char *text, bool clearDisplay = false, int textDelay = 0)
{
    const int maxColumns = 16;
    const int maxRows = 8;

    if (clearDisplay)
    {
        SeeedOled.clearDisplay();
    }

    int row = 0;
    int col = 0;

    while (*text)
    {
        if (col == maxColumns || *text == '\n')
        {
            row++;
            col = 0;
            if (row == maxRows)
            {
                delay(textDelay);
                SeeedOled.clearDisplay();
                row = 0;
            }
            if (*text == '\n')
            {
                text++;
                continue;
            }
        }

        SeeedOled.setTextXY(row, col);
        SeeedOled.putChar(*text);
        text++;
        col++;
    }

    delay(textDelay);
    if (clearDisplay)
    {
        SeeedOled.clearDisplay();
    }
}

void initialSetup()
{
    bool success = false;
    initOled();
    delay(100);
    printToOled("Setup beginnt in 5 Sekunden.", true, 5000);

    Serial.println("Captive Portal wird aufgesetzt.");
    printToOled("Captive Portal wird aufgesetzt...");
    setupWiFiManager();

    Serial.println("UUID wird generiert...");
    initializeUUID();

    Serial.println("Nanoleaf MDNS Lookup");

    Serial.println("Die Suche nach der Nanoleaf URL wird gestartet...");
    printToOled("Suche Nach Nanoleafs");
    while (!success)
    {
        SeeedOled.setTextXY(7, 0);
        SeeedOled.putChar('.');
        delay(500);
        success = ensureNanoleafURL();
    }
    printToOled("Nanoleafs wurden gefunden.", true, 5000);

    Serial.println("MQTT Verbindung wird aufgebaut...");
    setupMQTTClient();

    Serial.println("Nanoleaf Auth token generation");
    printToOled("Nanoleaf API aktivieren", true, 10000);
    attemptNanoleafConnection();
    printToOled("Nanoleafs erfolgreich verbunden.", true, 5000);
    SeeedOled.clearDisplay();
    SeeedOled.setTextXY(0, 0);
    SeeedOled.putString("FriendID: ");
    SeeedOled.setTextXY(2, 0);
    SeeedOled.putString(friendId);
    delay(60 * 1000);
    printToOled("Neustart: ", true, 5000);

    initialSetupDone = true;
    saveConfigToFile();

    Serial.println("Ersteinrichtung abgeschlossen. Der ESP wird neu gestartet...");
    ESP.restart();
}

void setup()
{
    Serial.begin(115200);
    loadConfigFromFile();

    if (!initialSetupDone)
    {
        initialSetup();
    }

    // loadConfigFromFile();
    connectToWifi(true);
    ensureNanoleafURL();
    setupMQTTClient();
    attemptNanoleafConnection();
    publishStatus();
    publishInitialHeartbeat();
}

void loop()
{
    mqttClient.loop();
    nanoleaf.processEvents();

    if (layoutChanged)
    {
        publishStatus();
        layoutChanged = false;
    }

    if (millis() - lastPublishTime >= PUBLISH_INTERVAL)
    {
        publishHeartbeat();
        lastPublishTime = millis();
    }
}
