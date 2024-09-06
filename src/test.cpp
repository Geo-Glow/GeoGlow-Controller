#include <ESP8266WiFi.h>
#include <ESP8266MDNS.h>

// Replace these with your network credentials
const char* _ssid = "Das tut W-Lan";      //  your network SSID (name)
const char* pass = "56515768934409322166";       // your network password
void _setup() {
  Serial.begin(115200);
  WiFi.begin(_ssid, pass);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  if (!MDNS.begin("esp8266")) {
    Serial.println("Error setting up MDNS responder!");
    return;
  }
  Serial.println("mDNS responder started");

  // Set a timer to trigger the mDNS scan
  ESP.wdtDisable();  // Disable watchdog timer as scanning might take long
}

void _loop() {
  Serial.println("Searching for mDNS services...");
  MDNS.update();

  int n = MDNS.queryService("nanoleafapi", "tcp"); // Query for NanoLeaf services
  Serial.println("mDNS scan done");

  if (n == 0) {
    Serial.println("No services found");
  } else {
    Serial.print(n);
    Serial.println(" service(s) found:");
    for (int i = 0; i < n; ++i) {
      Serial.print("  IP: ");
      Serial.println(MDNS.answerIP(i));
      Serial.print("  Port: ");
      Serial.println(MDNS.answerPort(i));
    }
  }

  delay(30000); // Wait 30 seconds before next scan
}