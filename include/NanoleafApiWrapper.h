#ifndef NanoleafApiWrapper_h
#define NanoleafApiWrapper_h

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#else
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#include <ArduinoJson.h>
#include <vector>

class NanoleafApiWrapper
{
public:
    explicit NanoleafApiWrapper(const WiFiClient &wifiClient);

    void setup(const char *nanoleafBaseUrl, const char *nanoleafAuthToken);

    bool isConnected();

    String generateToken();

    bool identify();

    String events();

    std::vector<String> getPanelIds();

    bool setPower(const bool &state);

    bool setStaticColors(const JsonObject &doc);

    bool registerEvents(const std::vector<int> &eventIds);

    void processEvents();

    typedef std::function<void()> LayoutChangeCallback;
    void setLayoutChangeCallback(LayoutChangeCallback callback);

private:
    bool sendRequest(
        const String &method,
        const String &endpoint,
        const JsonDocument *requestBody,
        JsonDocument *responseBody,
        bool useAuthToken);

    std::vector<String> triangleIds;
    String nanoleafBaseUrl;
    String nanoleafAuthToken;
    WiFiClient client;
    HTTPClient httpClient;
    WiFiClient *eventClient;
    bool registeredForEvents = false;
    LayoutChangeCallback layoutChangeCallback;
};

#endif
