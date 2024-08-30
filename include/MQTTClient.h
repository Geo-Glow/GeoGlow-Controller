#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <vector>
#include <ArduinoJson.h>
#include "TopicAdapter.h"

class MQTTClient {
public:
    explicit MQTTClient(WiFiClient &wifiClient);

    void setup(const char *mqttBroker, int mqttPort, const char *friendId, const char *deviceId);

    void loop();

    void publish(const char *topic, const JsonDocument &jsonPayload);

    void addTopicAdapter(TopicAdapter *adapter);

private:
    void reconnect();

    char *buildTopic(const TopicAdapter *adapter) const;

    void callback(char *topic, const byte *payload, unsigned int length) const;

    static bool matches(const char *subscribedTopic, const char *receivedTopic);

    PubSubClient client;
    const char *friendId{};
    const char *deviceId{};
    static std::vector<TopicAdapter *> topicAdapters;
};

#endif
