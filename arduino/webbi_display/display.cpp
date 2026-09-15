#include <Arduino.h>
#include "display.h"

// ============================================================
// CONFIG
// ============================================================

static const int NUM_DIGITS = 4;

// segment pins
static const int segPins[7] = {
  18,
  19,
  21,
  22,
  23,
  25,
  26
};

// FIXED: D4 = GPIO14
static const int digPins[4] = {
  32,
  33,
  27,
  14
};

// ============================================================
// THIS MATCHES YOUR WORKING TEST
// ============================================================

// segments
static const uint8_t SEG_ON  = HIGH;
static const uint8_t SEG_OFF = LOW;

// digits
static const uint8_t DIGIT_ON  = HIGH;
static const uint8_t DIGIT_OFF = LOW;

// multiplex timing
static const unsigned long DIGIT_HOLD_US = 2000;

// ============================================================
// SEGMENT PATTERNS
// ============================================================

static const uint8_t segPatterns[10][7] = {

  {1,1,1,1,1,1,0}, // 0
  {0,1,1,0,0,0,0}, // 1
  {1,1,0,1,1,0,1}, // 2
  {1,1,1,1,0,0,1}, // 3
  {0,1,1,0,0,1,1}, // 4
  {1,0,1,1,0,1,1}, // 5
  {1,0,1,1,1,1,1}, // 6
  {1,1,1,0,0,0,0}, // 7
  {1,1,1,1,1,1,1}, // 8
  {1,1,1,1,0,1,1}  // 9
};

// -1 = blank
static int displayBuffer[4] = {-1,-1,-1,-1};

// ============================================================

static void allDigitsOff() {

  for (int i = 0; i < NUM_DIGITS; i++) {
    digitalWrite(digPins[i], DIGIT_OFF);
  }
}

static void allSegmentsOff() {

  for (int i = 0; i < 7; i++) {
    digitalWrite(segPins[i], SEG_OFF);
  }
}

static void writeSegments(int value) {

  // blank digit
  if (value < 0 || value > 9) {
    allSegmentsOff();
    return;
  }

  for (int s = 0; s < 7; s++) {

    digitalWrite(
      segPins[s],
      segPatterns[value][s] ? SEG_ON : SEG_OFF
    );
  }
}

// ============================================================

void setupDisplay() {

  for (int i = 0; i < 7; i++) {
    pinMode(segPins[i], OUTPUT);
  }

  for (int i = 0; i < NUM_DIGITS; i++) {
    pinMode(digPins[i], OUTPUT);
  }

  allSegmentsOff();
  allDigitsOff();
}

// ============================================================
// NO LEADING ZEROS
// ============================================================
void setNumber(long value) {

  // limit to 4 digits
  value = value % 10000;

  // clear display
  for (int i = 0; i < NUM_DIGITS; i++) {
    displayBuffer[i] = -1;
  }

  // special case for 0
  if (value == 0) {
    displayBuffer[0] = 0;
    return;
  }

  // LEFT-TO-RIGHT filling
  int digits[4];
  int count = 0;

  while (value > 0 && count < NUM_DIGITS) {
    digits[count] = value % 10;
    value /= 10;
    count++;
  }

  // reverse into display buffer
  for (int i = 0; i < count; i++) {
    displayBuffer[i] = digits[count - 1 - i];
  }
}
// ============================================================

void refreshDisplay() {

  static int currentDigit = 0;
  static unsigned long lastSwitch = 0;

  if (micros() - lastSwitch < DIGIT_HOLD_US) {
    return;
  }

  lastSwitch = micros();

  // turn all off first
  allDigitsOff();
  allSegmentsOff();

  // find next ACTIVE digit
  int attempts = 0;

  while (
    displayBuffer[currentDigit] < 0 &&
    attempts < NUM_DIGITS
  ) {
    currentDigit = (currentDigit + 1) % NUM_DIGITS;
    attempts++;
  }

  // draw valid digit
  int value = displayBuffer[currentDigit];

  if (value >= 0 && value <= 9) {

    writeSegments(value);

    digitalWrite(digPins[currentDigit], DIGIT_ON);
  }

  // next digit
  currentDigit = (currentDigit + 1) % NUM_DIGITS;
}