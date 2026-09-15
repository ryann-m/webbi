#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "api_client.h"

volatile long currentVisitors = 0;

// HTTP (not HTTPS) for validation — avoids TLS complexity for now.
static const char* apiURL = "http://10.205.10.2:8090/api/visitors";

void fetchVisitorCount() {
  if (WiFi.status() != WL_CONNECTED) return;   // keep last value if offline

  HTTPClient http;
  http.setConnectTimeout(3000);   // bound the blocking time
  http.setTimeout(3000);

  if (!http.begin(apiURL)) return;

  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(512);
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) {
      long v = doc["requests"] | -1;          // only "requests" for now
      if (v >= 0) {
        currentVisitors = v;
        Serial.print("requests = ");
        Serial.println(v);
      }
    } else {
      Serial.print("JSON error: ");
      Serial.println(err.c_str());
    }
  } else {
    Serial.print("HTTP code: ");
    Serial.println(code);
  }
  http.end();
}