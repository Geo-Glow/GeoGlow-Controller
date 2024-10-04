#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include <PubSubClient.h>
#include <vector>
#include <ArduinoJson.h>
#include "TopicAdapter.h"

class MQTTClient
{
public:
    explicit MQTTClient(WiFiClient &wifiClient);

    void setup(const char *mqttBroker, int mqttPort, const char *friendId);

    void loop();

    void publish(const char *topic, const JsonDocument &jsonPayload);

    void addTopicAdapter(TopicAdapter *adapter);

private:
    void reconnect();

    String buildTopic(const TopicAdapter *adapter) const;

    static void staticCallback(char *topic, byte *payload, unsigned int length);

    void callback(char *topic, byte *payload, unsigned int length);

    static bool matches(const String &subscribedTopic, const String &receivedTopic);

    PubSubClient client;
    String friendId;
    static std::vector<TopicAdapter *> topicAdapters;
    static MQTTClient *instance;
};

#endif // MQTTCLIENT_H