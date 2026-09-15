#include <Arduino.h>

// Segment pins: A B C D E F G
int segPins[7] = {16, 17, 18, 19, 21, 22, 23}; // G not wired? leave as 23 or unused

// Digit pins: D1 D2 D3 D4 (only 4 used)
int digitPins[4] = {12, 14, 26, 33};

// 0 = ON (LOW), 1 = OFF (HIGH) for common anode
byte zero[7] = {0, 0, 0, 0, 0, 0, 1}; // displays "0"

void setup() {
  // Setup segment pins
  for (int i = 0; i < 7; i++) {
    pinMode(segPins[i], OUTPUT);
    digitalWrite(segPins[i], HIGH); // OFF
  }

  // Setup digit pins
  for (int i = 0; i < 4; i++) {
    pinMode(digitPins[i], OUTPUT);
    digitalWrite(digitPins[i], HIGH); // OFF
  }
}

void loop() {
  // VERY simple multiplexing for 4 digits
  for (int d = 0; d < 4; d++) {

    // turn all digits OFF
    for (int i = 0; i < 4; i++) {
      digitalWrite(digitPins[i], HIGH);
    }

    // set segments for "0"
    for (int s = 0; s < 7; s++) {
      digitalWrite(segPins[s], zero[s] ? HIGH : LOW);
    }

    // turn current digit ON
    digitalWrite(digitPins[d], LOW);

    delay(5); // small delay so it’s visible
  }
}