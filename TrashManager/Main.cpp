#include <Arduino.h>
#include <Servo.h>

Servo myServo;

const int irPin = 2;
const int servoPin = 9;

const int closedAngle = 0;
const int openAngle = 90;

int triggerCount = 0;
String currentStatus = "IDLE";
String obstacleState = "CLEAR";

// Embed the HTML/JS dashboard directly in Flash memory (PROGMEM) to save RAM
const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Arduino Gate Monitor</title>
  <style>
    body { font-family: sans-serif; background: #121212; color: #fff; text-align: center; padding: 30px; }
    .btn { background: #00adb5; color: #fff; border: none; padding: 12px 24px; font-size: 16px; border-radius: 6px; cursor: pointer; }
    .grid { display: flex; justify-content: center; gap: 20px; margin-top: 30px; }
    .card { background: #1e1e1e; border: 1px solid #333; border-radius: 8px; padding: 20px; width: 160px; }
    .card h3 { margin: 0 0 10px 0; font-size: 14px; color: #888; }
    .card div { font-size: 22px; font-weight: bold; color: #00adb5; }
  </style>
</head>
<body>
  <h2>Arduino USB Live Dashboard</h2>
  <button class="btn" id="connectBtn">Connect to Arduino</button>
  
  <div class="grid">
    <div class="card"><h3>STATUS</h3><div id="stat">WAITING</div></div>
    <div class="card"><h3>IR SENSOR</h3><div id="ir">CLEAR</div></div>
    <div class="card"><h3>SERVO</h3><div id="angle">0°</div></div>
    <div class="card"><h3>COUNT</h3><div id="count">0</div></div>
  </div>

  <script>
    let port, reader;
    document.getElementById('connectBtn').addEventListener('click', async () => {
      try {
        port = await navigator.serial.requestPort();
        await port.open({ baudRate: 9600 });
        document.getElementById('connectBtn').style.display = 'none';

        const textDecoder = new TextDecoderStream();
        port.readable.pipeTo(textDecoder.writable);
        reader = textDecoder.readable.getReader();

        let buffer = '';
        while (true) {
          const { value, done } = await reader.read();
          if (done) break;
          buffer += value;
          const lines = buffer.split('\n');
          buffer = lines.pop();

          for (const line of lines) {
            const trimmed = line.trim();
            if (trimmed.startsWith('{') && trimmed.endsWith('}')) {
              try {
                const data = JSON.parse(trimmed);
                document.getElementById('stat').innerText = data.status;
                document.getElementById('ir').innerText = data.obstacle;
                document.getElementById('angle').innerText = data.angle + '°';
                document.getElementById('count').innerText = data.count;
              } catch(e) {}
            }
          }
        }
      } catch (err) {
        alert('Serial connection failed: ' + err);
      }
    });
  </script>
</body>
</html>
)rawliteral";

void sendJson(int angle) {
  Serial.print("{\"status\":\"");
  Serial.print(currentStatus);
  Serial.print("\",\"obstacle\":\"");
  Serial.print(obstacleState);
  Serial.print("\",\"angle\":");
  Serial.print(angle);
  Serial.print(",\"count\":");
  Serial.print(triggerCount);
  Serial.println("}");
}

void setup() {
  pinMode(irPin, INPUT);
  myServo.attach(servoPin);
  myServo.write(closedAngle);
  Serial.begin(9600);
}

void loop() {
  // If laptop asks for the HTML source code, send it
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'H') {
      Serial.println(reinterpret_cast<const __FlashStringHelper *>(DASHBOARD_HTML));
    }
  }

  int sensorState = digitalRead(irPin);

  if (sensorState == LOW) {
    triggerCount++;
    obstacleState = "DETECTED";
    currentStatus = "OPENING";
    sendJson(openAngle);
    myServo.write(openAngle);
    delay(1000);

    currentStatus = "CLOSING";
    sendJson(closedAngle);
    myServo.write(closedAngle);
    delay(150);

    currentStatus = "IDLE";
    obstacleState = "CLEAR";
    sendJson(closedAngle);
  }

  delay(30);
}