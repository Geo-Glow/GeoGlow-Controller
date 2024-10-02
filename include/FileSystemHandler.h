#ifndef FILESYSTEMHANDLER_H
#define FILESYSTEMHANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

class FileSystemHandler
{
public:
    static bool loadConfigFromFile(const char *path, JsonDocument &jsonDoc, size_t jsonSize);
    static bool saveConfigToFile(const char *path, const JsonDocument &jsonDoc);
};

#endif // FILESYSTEMHANDLER_H