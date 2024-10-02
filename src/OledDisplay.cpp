#include "OledDisplay.h"

void OledDisplay::init()
{
    Wire.begin();
    SeeedOled.init();
    SeeedOled.clearDisplay();
    SeeedOled.setNormalDisplay();
    SeeedOled.setPageMode();
}

void OledDisplay::printToOled(const char *text, bool clearDisplay, int textDelay)
{
    const int maxColumns = 16;
    const int maxRows = 8;

    if (clearDisplay)
    {
        SeeedOled.clearDisplay();
    }

    int row = 0;
    int col = 0;

    while (*text)
    {
        if (col == maxColumns || *text == '\n')
        {
            row++;
            col = 0;
            if (row == maxRows)
            {
                delay(textDelay);
                SeeedOled.clearDisplay();
                row = 0;
            }
            if (*text == '\n')
            {
                text++;
                continue;
            }
        }

        SeeedOled.setTextXY(row, col);
        SeeedOled.putChar(*text);
        text++;
        col++;
    }

    delay(textDelay);
    if (clearDisplay)
    {
        SeeedOled.clearDisplay();
    }
}