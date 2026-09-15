#include <Arduino.h>

// =========================
// SEGMENT GPIO PINS
// =========================

const int segA = 18;
const int segB = 19;
const int segC = 21;
const int segD = 22;
const int segE = 23;
const int segF = 25;
const int segG = 26;

// =========================
// DIGIT ENABLE PINS
// =========================

const int digit1 = 32;
const int digit2 = 33;
const int digit3 = 27;
const int digit4 = 13;

// =========================
// DIGIT MAP
// A B C D E F G
// =========================

bool digits[10][7] = {

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

// =========================
// CLEAR ALL SEGMENTS
// =========================

void clearSegments() {

  digitalWrite(segA, LOW);
  digitalWrite(segB, LOW);
  digitalWrite(segC, LOW);
  digitalWrite(segD, LOW);
  digitalWrite(segE, LOW);
  digitalWrite(segF, LOW);
  digitalWrite(segG, LOW);
}

// =========================
// TURN ALL DIGITS OFF
// =========================

void clearDigits() {

  digitalWrite(digit1, LOW);
  digitalWrite(digit2, LOW);
  digitalWrite(digit3, LOW);
  digitalWrite(digit4, LOW);
}

// =========================
// SET SEGMENTS
// =========================

void setSegments(int number) {

  digitalWrite(segA, digits[number][0]);
  digitalWrite(segB, digits[number][1]);
  digitalWrite(segC, digits[number][2]);
  digitalWrite(segD, digits[number][3]);
  digitalWrite(segE, digits[number][4]);
  digitalWrite(segF, digits[number][5]);
  digitalWrite(segG, digits[number][6]);
}

// =========================
// DISPLAY ONE DIGIT
// =========================

void showDigit(int digitPin, int value) {

  // blank everything first
  clearDigits();
  clearSegments();

  // load segment pattern
  setSegments(value);

  // enable ONE display
  digitalWrite(digitPin, HIGH);

  delayMicroseconds(2000);

  // turn display back off
  digitalWrite(digitPin, LOW);
}

// =========================
// SETUP
// =========================

void setup() {

  pinMode(segA, OUTPUT);
  pinMode(segB, OUTPUT);
  pinMode(segC, OUTPUT);
  pinMode(segD, OUTPUT);
  pinMode(segE, OUTPUT);
  pinMode(segF, OUTPUT);
  pinMode(segG, OUTPUT);

  pinMode(digit1, OUTPUT);
  pinMode(digit2, OUTPUT);
  pinMode(digit3, OUTPUT);
  pinMode(digit4, OUTPUT);

  clearSegments();
  clearDigits();
}

// =========================
// MAIN LOOP
// =========================

void loop() {

  int n = 1234;

  int thousands = (n / 1000) % 10;
  int hundreds  = (n / 100) % 10;
  int tens      = (n / 10) % 10;
  int ones      = n % 10;

  showDigit(digit1, thousands);
  showDigit(digit2, hundreds);
  showDigit(digit3, tens);
  showDigit(digit4, ones);
}