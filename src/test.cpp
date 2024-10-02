/*#include <Wire.h>
#include <SeeedOLED.h>

// Define the maximum number of characters per line and maximum number of lines
const int maxColumns = 16; // Assume 16 columns (adjust based on your screen width)
const int maxRows = 8;     // Assume 8 rows (adjust based on your screen height)

void initOled()
{
  Wire.begin();
  SeeedOled.init();
  SeeedOled.clearDisplay();
  SeeedOled.setNormalDisplay();
  SeeedOled.setPageMode();
}

void printToOled(const char *text, bool clearDisplay = false, int textDelay = 0)
{
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

void initialSetup()
{
  bool success = false;
  initOled();
  delay(100);
  printToOled("Setup beginnt in 5 Sekunden.", true, 5000);

  Serial.println("Captive Portal wird aufgesetzt.");
  printToOled("Captive Portal wird aufgesetzt...");

  Serial.println("UUID wird generiert...");

  Serial.println("Nanoleaf MDNS Lookup");

  Serial.println("Die Suche nach der Nanoleaf URL wird gestartet...");
  printToOled("Suche Nach Nanoleafs");
  int counter = 0;
  while (!success)
  {
    SeeedOled.setTextXY(7, 0); // Move cursor to the last row to display loading dots
    SeeedOled.putChar('.');
    delay(500); // Wait a bit before next dot
    counter++;
    if (counter == 10)
    {
      success = true;
    }
  }

  Serial.println("MQTT Verbindung wird aufgebaut...");
  printToOled("MQTT Verbdg. wird aufgebaut...");

  Serial.println("Nanoleaf Auth token generation");
  // printToOled("Nanoleaf API aktivieren", true, 10000);

  SeeedOled.clearDisplay();
  SeeedOled.setTextXY(0, 0);
  SeeedOled.putString("FriendID: ");
  SeeedOled.setTextXY(2, 0);
  SeeedOled.putString("Nick@379435A9"); // Assuming friendId contains the correct string "Nick@79435A9D"
  delay(60 * 1000);
  printToOled("Neustart: ");

  Serial.println("Ersteinrichtung abgeschlossen. Der ESP wird neu gestartet...");
  ESP.restart();
}

void setup()
{
  // Initialize serial communication for debugging
  Serial.begin(115200);
  initialSetup();
}

void loop()
{
  // Your main code here
}*/