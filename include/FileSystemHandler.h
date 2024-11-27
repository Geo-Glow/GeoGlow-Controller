#ifndef FILESYSTEMHANDLER_H
#define FILESYSTEMHANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#if defined(ESP8266)
#include <LittleFS.h>
#define FILESYSTEM LittleFS
#elif defined(ESP32)
#include <LITTLEFS.h>
#define FILESYSTEM LITTLEFS
#endif

class FileSystemHandler
{
public:
    static bool loadConfigFromFile(const char *path, JsonDocument &jsonDoc, size_t jsonSize);
    static bool saveConfigToFile(const char *path, const JsonDocument &jsonDoc);
    static bool removeConfigFile(const char *path);
};

#endif // FILESYSTEMHANDLER_H