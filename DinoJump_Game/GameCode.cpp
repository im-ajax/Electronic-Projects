#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define IR_PIN 2

const unsigned char PROGMEM dino_bmp[] = {
  0x07, 0xF0, 0x07, 0xFE, 0x07, 0xFE, 0x07, 0xE0, 0x0F, 0xFC,
  0x7F, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xDF, 0xF8, 0x8F, 0xF0,
  0x07, 0xE0, 0x06, 0x60, 0x06, 0x60
};
#define DINO_W 16
#define DINO_H 13

const unsigned char PROGMEM cactus_bmp[] = {
  0x18, 0x18, 0x5A, 0x7E, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x18
};
#define CACTUS_W 8
#define CACTUS_H 10

const int GROUND_Y = 56;
const int DINO_X = 14;

// Tuned physics for high-frequency updates
float dino_y = GROUND_Y - DINO_H;
float dino_velocity = 0;
const float GRAVITY = 0.55;       // Scaled down for faster cycle
const float JUMP_FORCE = -5.8;    // Scaled down for faster cycle
bool is_jumping = false;

float cactus_x = SCREEN_WIDTH;
float cactus_speed = 2.0;         // Scaled down for faster cycle

unsigned long score = 0;
bool game_over = false;
unsigned long game_over_time = 0;

void resetGame() {
  dino_y = GROUND_Y - DINO_H;
  dino_velocity = 0;
  is_jumping = false;
  cactus_x = SCREEN_WIDTH;
  cactus_speed = 2.0;
  score = 0;
  game_over = false;
}

void setup() {
  pinMode(IR_PIN, INPUT);

  // Overclock I2C bus before display initialization
  Wire.begin();
  Wire.setClock(800000L); // 800 kHz Fast Mode+ overclock

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (true);
  }

  // Ensure clock remains at 800 kHz
  Wire.setClock(800000L);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 20);
  display.println(F("DINO RUNNER"));
  display.setCursor(12, 36);
  display.println(F("Max Performance"));
  display.display();
  delay(1200);

  resetGame();
}

void loop() {
  bool ir_detected = (digitalRead(IR_PIN) == LOW);

  if (game_over) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(35, 16);
    display.println(F("GAME OVER"));

    display.setCursor(30, 30);
    display.print(F("Score: "));
    display.print(score);

    display.setCursor(10, 46);
    display.println(F("Remove hand & wave"));
    display.display();

    if (millis() - game_over_time > 600) {
      if (digitalRead(IR_PIN) == HIGH) {
        while (digitalRead(IR_PIN) == HIGH) {
          delay(10);
        }
        delay(200);
        resetGame();
      }
    }
    return;
  }

  // Jump Trigger
  if (ir_detected && !is_jumping) {
    dino_velocity = JUMP_FORCE;
    is_jumping = true;
  }

  // Physics Update
  if (is_jumping) {
    dino_y += dino_velocity;
    dino_velocity += GRAVITY;

    if (dino_y >= (GROUND_Y - DINO_H)) {
      dino_y = GROUND_Y - DINO_H;
      is_jumping = false;
      dino_velocity = 0;
    }
  }

  // Obstacle Movement
  cactus_x -= cactus_speed;
  if (cactus_x + CACTUS_W < 0) {
    cactus_x = SCREEN_WIDTH;
    score += 10;
    if (score % 50 == 0 && cactus_speed < 4.5) {
      cactus_speed += 0.2;
    }
  }

  // Collision Detection
  int d_left = DINO_X + 2;
  int d_right = DINO_X + DINO_W - 2;
  int d_top = (int)dino_y + 2;
  int d_bottom = (int)dino_y + DINO_H;

  int c_left = (int)cactus_x + 1;
  int c_right = (int)cactus_x + CACTUS_W - 1;
  int c_top = GROUND_Y - CACTUS_H + 1;
  int c_bottom = GROUND_Y;

  if (d_right > c_left && d_left < c_right && d_bottom > c_top && d_top < c_bottom) {
    game_over = true;
    game_over_time = millis();
    return;
  }

  // Frame Render
  display.clearDisplay();
  display.drawFastHLine(0, GROUND_Y, SCREEN_WIDTH, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(85, 4);
  display.print(score);

  display.drawBitmap(DINO_X, (int)dino_y, dino_bmp, DINO_W, DINO_H, SSD1306_WHITE);
  display.drawBitmap((int)cactus_x, GROUND_Y - CACTUS_H, cactus_bmp, CACTUS_W, CACTUS_H, SSD1306_WHITE);

  // Push frame directly over 800kHz I2C bus (no software delay)
  display.display();
}

