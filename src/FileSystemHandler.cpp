#include "FileSystemHandler.h"

bool FileSystemHandler::loadConfigFromFile(const char *path, JsonDocument &jsonDoc, size_t jsonSize)
{
    if (!LittleFS.begin())
    {
        Serial.println("Failed to mount FS");
        return false;
    }

    if (!LittleFS.exists(path))
    {
        Serial.println("Config file does not exist");
        LittleFS.end();
        return false;
    }

    File configFile = LittleFS.open(path, "r");
    if (!configFile)
    {
        Serial.println("Failed to open config file");
        LittleFS.end();
        return false;
    }

    size_t size = configFile.size();
    if (size > jsonSize)
    {
        Serial.println("Config file is too large");
        configFile.close();
        LittleFS.end();
        return false;
    }

    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);
    configFile.close();
    LittleFS.end();

    DeserializationError error = deserializeJson(jsonDoc, buf.get());
    if (error)
    {
        Serial.println("Failed to parse JSON config file");
        return false;
    }

    Serial.println("Parsed JSON config");
    return true;
}

bool FileSystemHandler::saveConfigToFile(const char *path, const JsonDocument &jsonDoc)
{
    if (!LittleFS.begin())
    {
        Serial.println("Failed to mount FS for save");
        return false;
    }

    File configFile = LittleFS.open(path, "w");
    if (!configFile)
    {
        Serial.println("Failed to open config file for writing");
        LittleFS.end();
        return false;
    }

    if (serializeJson(jsonDoc, configFile) == 0)
    {
        Serial.println("Failed to write JSON to config file");
        configFile.close();
        LittleFS.end();
        return false;
    }

    Serial.println("Config saved successfully");
    configFile.close();
    LittleFS.end();
    return true;
}