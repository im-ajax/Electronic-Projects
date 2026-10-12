#include <Arduino.h>

// Pin Definitions
#define PIN_POT   1  // Potentiometer wiper (ADC1)
#define PIN_TOUCH 4  // TTP223 Touch sensor SIG (Fire)
#define PIN_IR    5  // IR Obstacle sensor OUT (Reload)

bool lastTouch = false;
bool lastIR = false;
bool firePending = false;
bool reloadPending = false;

unsigned long lastSerialSend = 0;
String incomingLine = "";

// Fast tracking variables
float smoothAdc = 0.0;
const float FILTER_ALPHA = 0.45;
const int ADC_MIN = 60;
const int ADC_MAX = 4020;
const float ROTATION_MULTIPLIER = 1.35f;

float readFastPot() {
  long sum = 0;
  for (int i = 0; i < 4; i++) {
    sum += analogRead(PIN_POT);
  }
  float avgRaw = (float)sum / 4.0;

  if (smoothAdc == 0.0) {
    smoothAdc = avgRaw;
  } else {
    smoothAdc = (FILTER_ALPHA * avgRaw) + ((1.0 - FILTER_ALPHA) * smoothAdc);
  }
  return smoothAdc;
}

void setup() {
  delay(1000);
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  pinMode(PIN_POT, INPUT);
  pinMode(PIN_TOUCH, INPUT_PULLDOWN); // TTP223 pulls HIGH on touch
  pinMode(PIN_IR, INPUT_PULLUP);      // IR drops LOW on obstacle detection
}

void loop() {
  // 1. Drain incoming serial lines
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') incomingLine = "";
    else incomingLine += c;
  }

  // 2. Read Touch Sensor (Fire missile)
  bool rawTouch = (digitalRead(PIN_TOUCH) == HIGH);
  if (rawTouch && !lastTouch) {
    firePending = true; // Latched
  }
  lastTouch = rawTouch;

  // 3. Read IR Sensor (Reload ammo - Active LOW)
  bool rawIR = (digitalRead(PIN_IR) == LOW);
  if (rawIR && !lastIR) {
    reloadPending = true; // Latched
  }
  lastIR = rawIR;

  // 4. Stream Telemetry at 50 Hz (~20ms)
  if (millis() - lastSerialSend >= 20) {
    lastSerialSend = millis();

    float currentAdc = readFastPot();
    if (currentAdc < ADC_MIN) currentAdc = ADC_MIN;
    if (currentAdc > ADC_MAX) currentAdc = ADC_MAX;

    float normalized = (currentAdc - ADC_MIN) / (float)(ADC_MAX - ADC_MIN);
    float angle = fmod(normalized * 360.0f * ROTATION_MULTIPLIER, 360.0f);
    if (angle < 0) angle += 360.0f;

    int fireSignal = firePending ? 1 : 0;
    int reloadSignal = reloadPending ? 1 : 0;

    firePending = false;
    reloadPending = false;

    // Send compact JSON telemetry packet
    Serial.printf("{\"angle\":%.1f,\"fire\":%d,\"reload\":%d}\n", angle, fireSignal, reloadSignal);
  }
}