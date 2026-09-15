#include <WiFi.h>

#include "wifi_manager.h"
#include "wifi.h"

void connectWiFi() {

  Serial.println();
  Serial.println("================================");
  Serial.println("Connecting to WiFi...");
  Serial.println("================================");

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");

    attempts++;

    if (attempts > 30) {

      Serial.println();
      Serial.println("WiFi connection FAILED");

      return;
    }
  }

  Serial.println();
  Serial.println("WiFi connected!");

  Serial.print("IP Address: ");

  Serial.println(WiFi.localIP());

  Serial.print("Signal strength (RSSI): ");

  Serial.println(WiFi.RSSI());
}
