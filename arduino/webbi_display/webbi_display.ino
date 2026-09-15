#include <Arduino.h>
#include "display.h"

// ============================================================

long counter = 0;

unsigned long lastIncrement = 0;

const unsigned long INCREMENT_INTERVAL_MS = 1000;

// ============================================================

void setup() {

  Serial.begin(115200);
  delay(100);

  setupDisplay();

  setNumber(counter);

  Serial.println();
  Serial.println("================================");
  Serial.println("WEBBI COUNTER TEST");
  Serial.println("================================");
}

// ============================================================

void loop() {

  // ALWAYS refresh multiplexing
  refreshDisplay();

  unsigned long now = millis();

  // increment once per second
  if (now - lastIncrement >= INCREMENT_INTERVAL_MS) {

    lastIncrement = now;

    counter++;

    // wrap back to 0 after 9999
    if (counter > 9999) {
      counter = 0;
    }

    setNumber(counter);

    Serial.print("Counter: ");
    Serial.println(counter);
  }
}