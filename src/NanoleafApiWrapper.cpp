#include "NanoleafApiWrapper.h"

NanoleafApiWrapper::NanoleafApiWrapper(const WiFiClient &wifiClient)
    : client(wifiClient) {
}


void NanoleafApiWrapper::setup(const char *nanoleafBaseUrl, const char *nanoleafAuthToken) {
    this->nanoleafBaseUrl = nanoleafBaseUrl;
    this->nanoleafAuthToken = nanoleafAuthToken;
}


bool NanoleafApiWrapper::sendRequest(const String &method, const String &endpoint, const JsonDocument *requestBody,
                                     JsonDocument *responseBody, const bool useAuthToken) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = nanoleafBaseUrl;

        if (useAuthToken) {
            url += "/api/v1/" + nanoleafAuthToken;
        } else {
            url += "/api/v1";
        }

        url += endpoint;

        http.begin(client, url);
        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = -1;

        if (method.equalsIgnoreCase("GET")) {
            httpResponseCode = http.GET();
        } else if (method.equalsIgnoreCase("POST")) {
            if (requestBody != nullptr) {
                String stringPayload;
                serializeJson(*requestBody, stringPayload);
                httpResponseCode = http.POST(stringPayload);
            } else {
                httpResponseCode = http.POST("");
            }
        } else if (method.equalsIgnoreCase("PUT")) {
            if (requestBody != nullptr) {
                String stringPayload;
                serializeJson(*requestBody, stringPayload);
                httpResponseCode = http.PUT(stringPayload);
            } else {
                httpResponseCode = http.PUT("");
            }
        }

        if (httpResponseCode > 0) {
            String response = http.getString();

            if (responseBody != nullptr) {
                deserializeJson(*responseBody, response);
            }

            http.end();
            return true;
        }
        Serial.print("Error on sending ");
        Serial.print(method);
        Serial.print(": ");
        Serial.println(httpResponseCode);
        http.end();
        return false;
    }
    Serial.println("WiFi Disconnected");
    return false;
}


bool NanoleafApiWrapper::isConnected() {
    JsonDocument jsonResponse;
    if (sendRequest("GET", "/", nullptr, &jsonResponse, true)) {
        if (jsonResponse["serialNo"] != nullptr) {
            return true;
        }
    }
    return false;
}


String NanoleafApiWrapper::generateToken() {
    JsonDocument jsonResponse;
    if (sendRequest("POST", "/new", nullptr, &jsonResponse, false)) {
        if (const String strPayload = jsonResponse["auth_token"]; strPayload != nullptr && strPayload != "null") {
            return strPayload;
        }
    }
    return "";
}


bool NanoleafApiWrapper::identify() {
    return sendRequest("PUT", "/identify", nullptr, nullptr, true);
}

std::vector<String> NanoleafApiWrapper::getPanelIds() {
    JsonDocument jsonResponse;

    std::vector<String> panelIds;

    if (sendRequest("GET", "/panelLayout/layout", nullptr, &jsonResponse, true) &&
        jsonResponse["positionData"] != nullptr
    ) {
        const size_t arraySize = jsonResponse["positionData"].size();

        for (size_t i = 0; i < arraySize; i++) {
            if (auto panelId = jsonResponse["positionData"][i]["panelId"].as<String>(); panelId != "0") {
                panelIds.push_back(panelId);
            }
        }
    }

    return panelIds;
}


bool NanoleafApiWrapper::setPower(const bool &state) {
    JsonDocument jsonPayload;
    jsonPayload["on"] = JsonObject();
    jsonPayload["on"]["value"] = state;
    return sendRequest("PUT", "/state", &jsonPayload, nullptr, true);
}


bool NanoleafApiWrapper::setStaticColors(const JsonObject &doc) {
    String animData = "";
    const unsigned int tileCount = doc.size();
    int totalDuration = 3600 * 1000; // 1 hour in milliseconds
    int steps = 2; // Number of steps (1 step per minute for simplicity)
    int stepDuration = totalDuration / steps; // Duration per step

    animData += String(tileCount * steps) + " ";

    for (JsonPair kv: doc) {
        String tileId = kv.key().c_str();
        auto rgb = kv.value().as<JsonArray>();

        int startR = rgb[0].as<int>();
        int startG = rgb[1].as<int>();
        int startB = rgb[2].as<int>();
        int endR = 0, endG = 0, endB = 0;

        for (int i = 0; i <= steps; ++i) {
            int R = startR + (endR - startR) * i / steps;
            int G = startG + (endG - startG) * i / steps;
            int B = startB + (endB - startB) * i / steps;
            int startTime = i * stepDuration;

            animData += tileId + " 2 " + String(R) + " " + String(G) + " " + String(B) + " 0 " + 
                        String(stepDuration) + " 0 0 0 0 360 ";
        }
    }

    Serial.println(animData);

    // Create the payload for sending the custom animation data
    StaticJsonDocument<1024> jsonPayload;
    jsonPayload["write"] = JsonObject();
    jsonPayload["write"]["command"] = "display";
    jsonPayload["write"]["version"] = "2.0";
    jsonPayload["write"]["animType"] = "custom";
    jsonPayload["write"]["animData"] = animData;
    jsonPayload["write"]["loop"] = false;

    // This portion from your example appears to try to add an empty palette; we'll leave this as-is
    jsonPayload["write"]["palette"] = JsonArray();
    jsonPayload["write"]["palette"].add(JsonObject());
    jsonPayload["write"]["palette"][0]["hue"] = 0; // Ensure this palette has some value
    jsonPayload["write"]["duration"] = totalDuration / 1000; // Duration in seconds

    return sendRequest("PUT", "/effects", &jsonPayload, nullptr, true);
}
