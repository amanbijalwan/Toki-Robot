
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// 1. WIFI CONFIGURATION
// =====================================================
const char* WIFI_SSID = "test wifi";//"YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "testwifi123";//"YOUR_WIFI_PASSWORD";

// =====================================================
// 2. OLED CONFIGURATION
// =====================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET
);

WebServer server(80);

// =====================================================
// 3. Toki STATE
// =====================================================
enum Mood {
  HAPPY,
  CURIOUS,
  SLEEPY,
  SURPRISED
};

Mood mood = HAPPY;

int lookDirection = 0;  // -1 left, 0 center, 1 right

bool eyesClosed = false;
bool idleEnabled = true;
bool gazeReturning = false;

unsigned long blinkStarted = 0;
unsigned long lastAutoBlink = 0;
unsigned long nextBlinkAt = 3000;

unsigned long nextGazeAt = 0;
unsigned long gazeReturnAt = 0;
unsigned long lastWiFiAttempt = 0;

bool oledReady = false;

// =====================================================
// 4. EMBEDDED WEB PAGE
// =====================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Toki Companion</title>
<style>
:root {
  color-scheme: dark;
  --bg: #10131a;
  --panel: #1b2230;
  --accent: #80e0cb;
  --text: #f1f4fa;
  --muted: #a4afc0;
}
* { box-sizing: border-box; }
body {
  margin: 0;
  padding: 24px 16px;
  background: var(--bg);
  color: var(--text);
  font-family: system-ui, sans-serif;
}
main { max-width: 650px; margin: auto; }
header { margin-bottom: 24px; }
h1 { color: var(--accent); margin: 0; font-size: 2rem; }
.subtitle { color: var(--muted); margin-top: 5px; }
.panel {
  background: var(--panel);
  border-radius: 18px;
  padding: 20px;
  margin-bottom: 16px;
}
h2 { font-size: 1rem; margin: 0 0 14px; }
.grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}
.row {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}
button {
  border: 1px solid #414c60;
  background: #303b4d;
  color: var(--text);
  border-radius: 11px;
  padding: 13px 10px;
  font-size: 1rem;
  cursor: pointer;
  flex: 1;
}
button:active { transform: scale(.98); }
button:focus-visible {
  outline: 2px solid var(--accent);
  outline-offset: 2px;
}
button.primary {
  background: #246c60;
  border-color: #246c60;
}
button.active { border-color: var(--accent); color: var(--accent); }
.status {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}
.label { color: var(--muted); font-size: .85rem; }
.value { font-weight: 650; margin-top: 4px; overflow-wrap: anywhere; }
#connection { color: var(--accent); }
#notice { color: var(--muted); min-height: 1.4em; margin-top: 12px; }
footer { color: var(--muted); font-size: .8rem; text-align: center; }
</style>
</head>
<body>
<main>
<header>
  <h1>Toki 🤖</h1>
  <div class="subtitle">Your little Wi-Fi companion</div>
</header>

<section class="panel">
  <h2>Device connection</h2>
  <div class="status">
    <div><div class="label">Connection</div>
      <div class="value" id="connection">Connecting...</div></div>
    <div><div class="label">IP address</div>
      <div class="value" id="ip">--</div></div>
  </div>
  <div id="notice" role="status" aria-live="polite"></div>
</section>

<section class="panel">
  <h2>Choose an expression</h2>
  <div class="grid">
    <button class="primary" onclick="command('/api/mood?value=happy')">😊 Happy</button>
    <button onclick="command('/api/mood?value=curious')">🤔 Curious</button>
    <button onclick="command('/api/mood?value=sleepy')">😴 Sleepy</button>
    <button onclick="command('/api/mood?value=surprised')">😮 Surprised</button>
  </div>
</section>

<section class="panel">
  <h2>Eye movement</h2>
  <div class="row">
    <button onclick="command('/api/gaze?value=left')">← Left</button>
    <button onclick="command('/api/gaze?value=center')">Center</button>
    <button onclick="command('/api/gaze?value=right')">Right →</button>
  </div>
  <div class="row" style="margin-top:10px">
    <button onclick="command('/api/blink')">Blink</button>
  </div>
</section>

<section class="panel">
  <h2>Autonomous behaviour</h2>
  <div class="row">
    <button class="primary" onclick="command('/api/idle?enabled=1')">Idle ON</button>
    <button onclick="command('/api/idle?enabled=0')">Idle OFF</button>
  </div>
</section>

<section class="panel">
  <h2>Current state</h2>
  <div class="status">
    <div><div class="label">Mood</div><div class="value" id="mood">--</div></div>
    <div><div class="label">Gaze</div><div class="value" id="gaze">--</div></div>
    <div><div class="label">Idle behaviour</div><div class="value" id="idle">--</div></div>
    <div><div class="label">Wi-Fi signal</div><div class="value" id="signal">--</div></div>
  </div>
</section>

<footer>Toki Stage 3 · ESP32-hosted interface</footer>
</main>

<script>
const notice = document.getElementById('notice');

async function command(path) {
  notice.textContent = 'Sending command...';
  try {
    const response = await fetch(path, {cache: 'no-store'});
    const data = await response.json();

    if (!response.ok || !data.ok) {
      throw new Error(data.error || 'Command failed');
    }

    notice.textContent = data.message || 'Command sent';
    await refreshState();
  } catch (e) {
    notice.textContent = 'Request failed: ' + e.message;
  }
}

async function refreshState() {
  try {
    const response = await fetch('/api/state', {cache: 'no-store'});
    if (!response.ok) throw new Error('Device unavailable');

    const s = await response.json();
    document.getElementById('connection').textContent = 'Online';
    document.getElementById('ip').textContent = s.ip;
    document.getElementById('mood').textContent = s.mood;
    document.getElementById('gaze').textContent = s.gaze;
    document.getElementById('idle').textContent = s.idle ? 'On' : 'Off';
    document.getElementById('signal').textContent = s.rssi + ' dBm';
  } catch (e) {
    document.getElementById('connection').textContent = 'Unavailable';
  }
}

refreshState();
setInterval(refreshState, 2500);
</script>
</body>
</html>
)rawliteral";

// =====================================================
// 5. OLED FACE RENDERER
// =====================================================
const char* moodName() {
  switch (mood) {
    case HAPPY: return "happy";
    case CURIOUS: return "curious";
    case SLEEPY: return "sleepy";
    case SURPRISED: return "surprised";
  }
  return "unknown";
}

void drawFace() {
  if (!oledReady) return;

  display.clearDisplay();

  if (eyesClosed || mood == SLEEPY) {
    display.drawLine(27, 31, 47, 31, SSD1306_WHITE);
    display.drawLine(81, 31, 101, 31, SSD1306_WHITE);
  }
  else if (mood == SURPRISED) {
    display.drawCircle(37, 29, 12, SSD1306_WHITE);
    display.drawCircle(91, 29, 12, SSD1306_WHITE);

    display.fillCircle(
      37 + lookDirection * 3, 29, 4, SSD1306_WHITE
    );
    display.fillCircle(
      91 + lookDirection * 3, 29, 4, SSD1306_WHITE
    );
  }
  else {
    display.fillRoundRect(
      25, 18, 24, 25, 8, SSD1306_WHITE
    );
    display.fillRoundRect(
      79, 18, 24, 25, 8, SSD1306_WHITE
    );

    int pupilOffset = lookDirection * 5;

    display.fillCircle(
      37 + pupilOffset, 30, 5, SSD1306_BLACK
    );
    display.fillCircle(
      91 + pupilOffset, 30, 5, SSD1306_BLACK
    );
  }

  switch (mood) {
    case HAPPY:
      display.drawLine(51, 44, 57, 49, SSD1306_WHITE);
      display.drawLine(57, 49, 64, 51, SSD1306_WHITE);
      display.drawLine(64, 51, 71, 49, SSD1306_WHITE);
      display.drawLine(71, 49, 77, 44, SSD1306_WHITE);
      break;

    case CURIOUS:
      display.drawCircle(64, 48, 4, SSD1306_WHITE);
      break;

    case SLEEPY:
      display.drawLine(56, 49, 72, 49, SSD1306_WHITE);
      break;

    case SURPRISED:
      display.drawCircle(64, 48, 6, SSD1306_WHITE);
      break;
  }

  display.display();
}

void setMood(Mood newMood) {
  mood = newMood;
  eyesClosed = false;
  drawFace();
}

// =====================================================
// 6. WEB API
// =====================================================
void sendResult(bool ok, const char* message,
                int statusCode = 200) {
  String json = "{\"ok\":";
  json += ok ? "true" : "false";
  json += ",\"message\":\"";
  json += message;
  json += "\"}";

  server.send(statusCode, "application/json", json);
}

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleState() {
  String gaze = "center";

  if (lookDirection < 0) gaze = "left";
  if (lookDirection > 0) gaze = "right";

  String json = "{";
  json += "\"mood\":\"";
  json += moodName();
  json += "\",\"gaze\":\"";
  json += gaze;
  json += "\",\"idle\":";
  json += idleEnabled ? "true" : "false";
  json += ",\"ip\":\"";
  json += WiFi.localIP().toString();
  json += "\",\"rssi\":";
  json += String(WiFi.RSSI());
  json += "}";

  server.send(200, "application/json", json);
}

void handleMood() {
  if (!server.hasArg("value")) {
    sendResult(false, "Missing mood", 400);
    return;
  }

  String value = server.arg("value");
  value.toLowerCase();

  if (value == "happy") setMood(HAPPY);
  else if (value == "curious") setMood(CURIOUS);
  else if (value == "sleepy") setMood(SLEEPY);
  else if (value == "surprised" || value == "surprise") {
    setMood(SURPRISED);
  }
  else {
    sendResult(false, "Unknown mood", 400);
    return;
  }

  sendResult(true, "Expression changed");
}

void handleGaze() {
  if (!server.hasArg("value")) {
    sendResult(false, "Missing gaze", 400);
    return;
  }

  String value = server.arg("value");
  value.toLowerCase();

  if (value == "left") lookDirection = -1;
  else if (value == "right") lookDirection = 1;
  else if (value == "center") lookDirection = 0;
  else {
    sendResult(false, "Unknown gaze direction", 400);
    return;
  }

  gazeReturning = false;
  nextGazeAt = millis() + random(2500, 6000);

  drawFace();
  sendResult(true, "Gaze updated");
}

void handleIdle() {
  if (!server.hasArg("enabled")) {
    sendResult(false, "Missing enabled parameter", 400);
    return;
  }

  String value = server.arg("enabled");

  if (value == "1") {
    idleEnabled = true;
    lookDirection = 0;
    gazeReturning = false;
    nextGazeAt = millis() + 1500;
  }
  else if (value == "0") {
    idleEnabled = false;
    lookDirection = 0;
    gazeReturning = false;
  }
  else {
    sendResult(false, "Use enabled=1 or enabled=0", 400);
    return;
  }

  drawFace();
  sendResult(true, idleEnabled
    ? "Idle behaviour enabled"
    : "Idle behaviour disabled");
}

void handleBlink() {
  eyesClosed = true;
  blinkStarted = millis();
  drawFace();
  sendResult(true, "Blink triggered");
}

void handleNotFound() {
  sendResult(false, "Endpoint not found", 404);
}

void configureRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/mood", HTTP_GET, handleMood);
  server.on("/api/gaze", HTTP_GET, handleGaze);
  server.on("/api/idle", HTTP_GET, handleIdle);
  server.on("/api/blink", HTTP_GET, handleBlink);
  server.onNotFound(handleNotFound);
}

// =====================================================
// 7. AUTONOMOUS BLINKING AND GAZE
// =====================================================
void updateBlink() {
  unsigned long now = millis();

  // A manual blink or an automatic blink lasts briefly.
  if (eyesClosed) {
    if (now - blinkStarted >= 140) {
      eyesClosed = false;
      lastAutoBlink = now;
      nextBlinkAt = random(2000, 5000);
      drawFace();
    }
    return;
  }

  if (mood != SLEEPY &&
      now - lastAutoBlink >= nextBlinkAt) {
    eyesClosed = true;
    blinkStarted = now;
    drawFace();
  }
}

void updateIdle() {
  if (!idleEnabled) return;

  unsigned long now = millis();

  if (gazeReturning && now >= gazeReturnAt) {
    lookDirection = 0;
    gazeReturning = false;
    nextGazeAt = now + random(2500, 6000);
    drawFace();
    return;
  }

  if (!gazeReturning && !eyesClosed &&
      mood != SLEEPY && now >= nextGazeAt) {
    lookDirection = random(0, 2) == 0 ? -1 : 1;
    gazeReturning = true;
    gazeReturnAt = now + random(450, 1000);
    drawFace();
  }
}

// =====================================================
// 8. WIFI CONNECTION
// =====================================================
void connectWiFi() {
  Serial.println();
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long started = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - started < 20000) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected!");
    Serial.print("Toki IP address: http://");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength: ");
    Serial.println(WiFi.RSSI());
  } else {
    Serial.println("Wi-Fi connection timed out.");
    Serial.println("The ESP32 will retry in the background.");
  }
}

void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  unsigned long now = millis();

  if (now - lastWiFiAttempt >= 10000) {
    lastWiFiAttempt = now;
    Serial.println("Retrying Wi-Fi connection...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

// =====================================================
// 9. SETUP AND MAIN LOOP
// =====================================================
void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  oledReady = display.begin(
    SSD1306_SWITCHCAPVCC, OLED_ADDRESS
  );

  if (oledReady) {
    randomSeed(micros());

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(36, 27);
    display.println("Toki online");
    display.display();

    nextBlinkAt = random(2000, 5000);
    lastAutoBlink = millis();
    nextGazeAt = millis() + random(2500, 6000);
  } else {
    Serial.println("OLED initialization failed!");
    Serial.println("Web server will still start.");
  }

  connectWiFi();

  configureRoutes();
  server.begin();

  Serial.println("HTTP server started.");
  Serial.println("Open the printed IP address in your browser.");
}

void loop() {
  server.handleClient();

  updateBlink();
  updateIdle();

  maintainWiFi();

  // Keep the main loop responsive.
  delay(2);
}
