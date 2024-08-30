#ifndef TOPICADAPTER_H
#define TOPICADAPTER_H

#include <ArduinoJson.h>

class TopicAdapter {
public:
    virtual ~TopicAdapter() = default;

    [[nodiscard]] virtual const char *getTopic() const = 0;

    virtual void callback(char *topic, const JsonObject &payload, unsigned int length) = 0;
};

#endif
