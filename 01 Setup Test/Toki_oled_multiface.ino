
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- Hardware ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

#define TOUCH_PIN 27
#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// ---------- toki's moods ----------
enum Mood {
  HAPPY,
  CURIOUS,
  SLEEPY,
  SURPRISED
};

Mood mood = HAPPY;

bool eyesClosed = false;
bool lastTouch = false;
//UART Test without touch
bool cmdChange = false;

unsigned long lastBlink = 0;
unsigned long blinkDuration = 0;
unsigned long lastMoodChange = 0;

unsigned long nextBlink = 2500;

// ---------- Face drawing ----------
void drawFace() {
  display.clearDisplay();

  if (eyesClosed) {
    // Closed eyes
    display.drawLine(28, 29, 48, 29, SSD1306_WHITE);
    display.drawLine(80, 29, 100, 29, SSD1306_WHITE);
  } else {
    switch (mood) {
      case HAPPY:
        display.fillRoundRect(
          25, 19, 24, 23, 9, SSD1306_WHITE
        );
        display.fillRoundRect(
          79, 19, 24, 23, 9, SSD1306_WHITE
        );
        break;

      case CURIOUS:
        display.fillCircle(38, 29, 12, SSD1306_WHITE);
        display.fillCircle(89, 29, 9, SSD1306_WHITE);
        break;

      case SLEEPY:
        display.fillRoundRect(
          27, 27, 22, 9, 4, SSD1306_WHITE
        );
        display.fillRoundRect(
          80, 27, 22, 9, 4, SSD1306_WHITE
        );
        break;

      case SURPRISED:
        display.drawCircle(38, 29, 12, SSD1306_WHITE);
        display.drawCircle(90, 29, 12, SSD1306_WHITE);
        display.fillCircle(38, 29, 5, SSD1306_WHITE);
        display.fillCircle(90, 29, 5, SSD1306_WHITE);
        break;
    }
  }

  // Mouth changes with mood
  switch (mood) {
    case HAPPY:
      display.drawLine(51, 45, 57, 50, SSD1306_WHITE);
      display.drawLine(57, 50, 64, 52, SSD1306_WHITE);
      display.drawLine(64, 52, 71, 50, SSD1306_WHITE);
      display.drawLine(71, 50, 77, 45, SSD1306_WHITE);
      break;

    case CURIOUS:
      display.drawCircle(64, 49, 4, SSD1306_WHITE);
      break;

    case SLEEPY:
      display.drawLine(55, 49, 73, 49, SSD1306_WHITE);
      break;

    case SURPRISED:
      display.drawCircle(64, 49, 6, SSD1306_WHITE);
      break;
  }

  display.display();
}

void changeMood() {
  mood = static_cast<Mood>((mood + 1) % 4);
  lastMoodChange = millis();
  drawFace();
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  pinMode(TOUCH_PIN, INPUT);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {
    Serial.println("OLED not found. Check wiring/address.");
    while (true) {
      delay(100);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(36, 27);
  display.println("Hi! I'm toki");
  display.display();

  delay(1200);

  randomSeed(micros());
  nextBlink = random(2000, 5000);
  lastBlink = millis();

  drawFace();
  Serial.println("toki is ready!");
}

// ---------- Main loop ----------
void loop() {
  unsigned long now = millis();

  // Touch sensor: one mood change per touch
  bool touch = digitalRead(TOUCH_PIN);

  if (touch && !lastTouch) {
    changeMood();
    Serial.println("Touch detected!");
  }

  lastTouch = touch;

  // Test on UART without touch ++
  if (Serial.available() > 0){
    char incomingChar = Serial.read();
    if (incomingChar == '1'){
    cmdChange = true;
    }
  }
  if (cmdChange == true){
    changeMood();
    Serial.println("Mood Changed CMD!");
    cmdChange = false;
  }
  // Test on UART without touch --


  // Automatic blink
  if (!eyesClosed && now - lastBlink >= nextBlink) {
    eyesClosed = true;
    blinkDuration = now;
    drawFace();
  }

  if (eyesClosed && now - blinkDuration >= 140) {
    eyesClosed = false;
    lastBlink = now;
    nextBlink = random(2000, 5000);
    drawFace();
  }
}


