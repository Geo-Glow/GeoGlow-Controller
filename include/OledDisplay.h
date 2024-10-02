#ifndef OLEDDISPLAY_H
#define OLEDDISPLAY_H

#include <Wire.h>
#include <SeeedOLED.h>

class OledDisplay
{
public:
    void init();
    void printToOled(const char *text, bool clearDisplay = false, int textDelay = 0);
};

#endif // OLEDDISPLAY_H