#include <Arduino.h>
const int PIN_IR_LEFT  = 2;
const int PIN_IR_RIGHT = 3;
char lastState = 'N';
void setup() {
  Serial.begin(115200);
  pinMode(PIN_IR_LEFT, INPUT_PULLUP);
  pinMode(PIN_IR_RIGHT, INPUT_PULLUP);
}
void loop() {
  bool left = (digitalRead(PIN_IR_LEFT) == LOW);
  bool right = (digitalRead(PIN_IR_RIGHT) == LOW);
  char currentState = 'N';
  if (left && !right) {
    currentState = 'L'; // Left steer
    Serial.println("Left Steering");
  } else if (right && !left) {
    currentState = 'R'; // Right steer
    Serial.println("Right Steering");
  }
  // Only send when sensor state changes to keep serial buffer clear
  if (currentState != lastState) {
    lastState = currentState;
    Serial.println(currentState);
  }
  delay(15);
}