#ifndef COLORPALETTEADAPTER_H
#define COLORPALETTEADAPTER_H

#include "TopicAdapter.h"
#include "NanoleafApiWrapper.h"

class ColorPaletteAdapter final : public TopicAdapter {
public:
    explicit ColorPaletteAdapter(NanoleafApiWrapper &nanoleaf): nanoleaf(nanoleaf), topic("color") {
    }

    [[nodiscard]] const char *getTopic() const override {
        return topic;
    }

    void callback(char *topic, const JsonObject &payload, unsigned int length) override {
        nanoleaf.setStaticColors(payload);
    }

private:
    NanoleafApiWrapper &nanoleaf;
    const char *topic;
};

#endif
