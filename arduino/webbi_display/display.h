#ifndef DISPLAY_H
#define DISPLAY_H

void setupDisplay();
void setNumber(long value);   // load value into the display buffer
void refreshDisplay();        // call as often as possible in loop()

#endif