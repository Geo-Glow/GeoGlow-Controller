#include "MQTTClient.h"

std::vector<TopicAdapter *> MQTTClient::topicAdapters;

MQTTClient::MQTTClient(WiFiClient &wifiClient)
    : client(wifiClient) {
}

void MQTTClient::setup(const char *mqttBroker, const int mqttPort, const char *friendId, const char *deviceId) {
    client.setServer(mqttBroker, mqttPort);

    auto callback = [this](char *topic, const byte *payload, const unsigned int length) {
        this->callback(topic, payload, length);
    };

    std::function<void(char *, uint8_t *, unsigned int)> function = callback;

    client.setCallback(callback);
    client.setBufferSize(2048);

    this->friendId = friendId;
    this->deviceId = deviceId;
}

void MQTTClient::loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
}

void MQTTClient::reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        auto mqttClientId = "GeoGlow-" + String(this->friendId) + "-" + String(this->deviceId);
        if (client.connect(mqttClientId.c_str())) {
            Serial.println("connected: " + mqttClientId);
            for (const auto adapter: topicAdapters) {
                client.subscribe(buildTopic(adapter));
            }
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void MQTTClient::publish(const char *topic, const JsonDocument &jsonPayload) {
    if (client.connected()) {
        char buffer[512];
        const size_t n = serializeJson(jsonPayload, buffer);
        client.publish(topic, buffer, n);
    } else {
        Serial.println("MQTT client not connected. Unable to publish message.");
    }
}

char *MQTTClient::buildTopic(const TopicAdapter *adapter) const {
    const size_t topicLength = strlen("GeoGlow/") +
                               strlen(friendId) + 1 +
                               strlen(deviceId) + 1 +
                               strlen(adapter->getTopic()) + 1;

    const auto topic = static_cast<char *>(malloc(topicLength));

    if (topic == nullptr) {
        return nullptr;
    }

    strcpy(topic, "GeoGlow/");
    strcat(topic, friendId);
    strcat(topic, "/");
    strcat(topic, deviceId);
    strcat(topic, "/");
    strcat(topic, adapter->getTopic());

    return topic;
}


void MQTTClient::addTopicAdapter(TopicAdapter *adapter) {
    topicAdapters.push_back(adapter);
    if (client.connected()) {
        client.subscribe(buildTopic(adapter));
    }
}

void MQTTClient::callback(char *topic, const byte *payload, const unsigned int length) const {
    char payloadBuffer[length + 1];
    memcpy(payloadBuffer, payload, length);
    payloadBuffer[length] = '\0';

    JsonDocument jsonDocument;

    if (deserializeJson(jsonDocument, payloadBuffer)) {
        Serial.print("Unhandled message [");
        Serial.print(topic);
        Serial.print("] ");
        Serial.println(payloadBuffer);
        return;
    }

    for (const auto adapter: topicAdapters) {
        if (matches(buildTopic(adapter), topic)) {
            adapter->callback(topic, jsonDocument.as<JsonObject>(), length);
            return;
        }
    }

    Serial.print("Unhandled message [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(payloadBuffer);
}

bool MQTTClient::matches(const char *subscribedTopic, const char *receivedTopic) {
    if (const char *wildCardPos = strchr(subscribedTopic, '#'); wildCardPos != nullptr) {
        if (wildCardPos[1] == '\0') {
            size_t subscribedTopicLength = wildCardPos - subscribedTopic;
            if (subscribedTopicLength > 0 && subscribedTopic[subscribedTopicLength - 1] == '/') {
                subscribedTopicLength--;
            }
            return strncmp(subscribedTopic, receivedTopic, subscribedTopicLength) == 0;
        }
        return false;
    }

    if (const char *plusPos = strchr(subscribedTopic, '+'); plusPos != nullptr) {
        const char *slashPos = strchr(receivedTopic, '/');
        if (slashPos == nullptr) {
            return true;
        }
        return strncmp(subscribedTopic, receivedTopic, plusPos - subscribedTopic) == 0 &&
               strcmp(plusPos + 1, slashPos + 1) == 0;
    }

    return strcmp(subscribedTopic, receivedTopic) == 0;
}
