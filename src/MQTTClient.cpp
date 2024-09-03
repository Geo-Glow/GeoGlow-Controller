#include "MQTTClient.h"

std::vector<TopicAdapter *> MQTTClient::topicAdapters;
MQTTClient* MQTTClient::instance = nullptr;

MQTTClient::MQTTClient(WiFiClient &wifiClient)
    : client(wifiClient) {
}

void MQTTClient::setup(const char *mqttBroker, const int mqttPort, const char *friendId) {
    client.setServer(mqttBroker, mqttPort);

    // Assign this instance to the static instance pointer
    instance = this;

    // Set the static callback
    client.setCallback(staticCallback);

    client.setBufferSize(2048);

    this->friendId = friendId;
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
        String mqttClientId = "GeoGlow-" + this->friendId;
        if (client.connect(mqttClientId.c_str())) {
            Serial.println("connected: " + mqttClientId);
            for (const auto adapter: topicAdapters) {
                client.subscribe(buildTopic(adapter).c_str());
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
        size_t n = serializeJson(jsonPayload, buffer);
        client.publish(topic, buffer, n);
    } else {
        Serial.println("MQTT client not connected. Unable to publish message.");
    }
}

String MQTTClient::buildTopic(const TopicAdapter *adapter) const {
    String topic = "GeoGlow/";
    topic += friendId + "/";
    topic += adapter->getTopic();
    return topic;
}

void MQTTClient::addTopicAdapter(TopicAdapter *adapter) {
    topicAdapters.push_back(adapter);
    if (client.connected()) {
        client.subscribe(buildTopic(adapter).c_str());
    }
}

void MQTTClient::staticCallback(char *topic, byte *payload, unsigned int length) {
    // Use the static instance pointer to call the instance method
    if (instance) {
        instance->callback(topic, payload, length);
    }
}

void MQTTClient::callback(char *topic, byte *payload, unsigned int length) {
    char payloadBuffer[length + 1];
    memcpy(payloadBuffer, payload, length);
    payloadBuffer[length] = '\0';

    DynamicJsonDocument jsonDocument(1024);
    
    DeserializationError error = deserializeJson(jsonDocument, payloadBuffer);
    if (error) {
        Serial.print("Failed to parse JSON payload: ");
        Serial.println(error.c_str());
        return;
    }

    String receivedTopic = String(topic);
    for (const auto adapter: topicAdapters) {
        if (matches(buildTopic(adapter), receivedTopic)) {
            adapter->callback(topic, jsonDocument.as<JsonObject>(), length);
            return;
        }
    }

    Serial.print("Unhandled message [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(payloadBuffer);
}

bool MQTTClient::matches(const String &subscribedTopic, const String &receivedTopic) {
    if (subscribedTopic.endsWith("#")) {
        String baseTopic = subscribedTopic.substring(0, subscribedTopic.length() - 1);
        return receivedTopic.startsWith(baseTopic);
    } else if (subscribedTopic.indexOf('+') >= 0) {
        int plusPos = subscribedTopic.indexOf('+');
        String preWildcard = subscribedTopic.substring(0, plusPos);
        String postWildcard = subscribedTopic.substring(plusPos + 1);
        if (receivedTopic.startsWith(preWildcard) && receivedTopic.endsWith(postWildcard)) {
            return true;
        }
    }
    return subscribedTopic == receivedTopic;
}