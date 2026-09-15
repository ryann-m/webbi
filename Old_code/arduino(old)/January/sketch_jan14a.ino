// Segment pins: A B C D E F G
int segPins[7] = {23, 22, 21, 19, 18, 17, 16};

// Digit pins: DIG1 DIG2 DIG3 DIG4
int digPins[4] = {25, 26, 27, 14};

// Pattern for "0" on a COMMON ANODE display:
// LOW = ON, HIGH = OFF
// A B C D E F G
int zero[7] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW};

void setup() {
  // segment pins
  for (int i = 0; i < 7; i++) {
    pinMode(segPins[i], OUTPUT);
    digitalWrite(segPins[i], HIGH); // segments OFF initially (common anode)
  }

  // digit pins
  for (int i = 0; i < 4; i++) {
    pinMode(digPins[i], OUTPUT);
    digitalWrite(digPins[i], LOW);  // digits OFF initially (common anode)
  }
}

void loop() {
  for (int d = 0; d < 4; d++) {

    // turn all digits OFF
    for (int i = 0; i < 4; i++) {
      digitalWrite(digPins[i], LOW);
    }

    // set segments to show "0"
    for (int s = 0; s < 7; s++) {
      digitalWrite(segPins[s], zero[s]);
    }

    // turn ON just this digit
    digitalWrite(digPins[d], HIGH);

    // small delay so it's visible
    delay(4);
  }
}