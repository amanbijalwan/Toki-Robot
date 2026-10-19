// To test eyes on OLED screen, for Toki.
// Ensue following libraries are installed in Arduino IDE.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  
  // Initialize SSD1306 display with I2C address 0x3C
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  // Draw Mimo's initial face (happy eyes)
  drawEyes(true);
}

void loop() {
  // Blink every 2.5 seconds
  delay(2500);
  drawBlink();
  delay(200);
  drawEyes(true);
}

void drawEyes(bool open) {
  display.clearDisplay();
  if (open) {
    // Left Eye
    display.fillRoundRect(30, 20, 20, 24, 4, SSD1306_WHITE);
    // Right Eye
    display.fillRoundRect(78, 20, 20, 24, 4, SSD1306_WHITE);
  }
  display.display();
}

void drawBlink() {
  display.clearDisplay();
  // Left Wink line
  display.fillRect(30, 30, 20, 4, SSD1306_WHITE);
  // Right Wink line
  display.fillRect(78, 30, 20, 4, SSD1306_WHITE);
  display.display();
}
