// ============================================================
// ESP32 RoboEyes Enhanced v5.1 - Persistent Settings
// ============================================================
// Touch, Animations, Clock, Melodies, Splash, NVS Storage
// ============================================================

#include <Adafruit_GFX.h>
#include <Adafruit_HTU21DF.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Wire.h>

#ifdef DEFAULT
#undef DEFAULT
#endif
#include <FluxGarage_RoboEyes.h>

#include "dashboard.h"

// ==========================================
// PINAGEM
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 16
#define I2C_SDA 5
#define I2C_SCL 4
#define BMI160_ADDR 0x69
#define BUZZER_PIN 0
#define LED_R 13
#define LED_G 12
#define LED_B 14
#define TOUCH_PIN 15
#define TOUCH_THRESHOLD 30

// ==========================================
// WiFi AP
// ==========================================
const char *AP_SSID = "RoboEyes";
const char *AP_PASS = "roboeyes123";

// ==========================================
// BMI160
// ==========================================
#define BMI160_REG_CHIP_ID 0x00
#define BMI160_REG_CMD 0x7E
#define BMI160_CHIP_ID_VAL 0xD1
#define BMI160_CMD_ACC_NORMAL 0x11
#define BMI160_CMD_GYR_NORMAL 0x15
#define BMI160_CMD_SOFT_RESET 0xB6

// ==========================================
// OBJETOS GLOBAIS
// ==========================================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RoboEyes<Adafruit_SSD1306> roboEyes(display);
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
AsyncWebServer server(80);
Preferences prefs;

// ==========================================
// ESTADO DOS SENSORES
// ==========================================
bool bmi160Ok = false;
bool htuOk = false;
float currentTemp = 25.0;
float currentHum = 50.0;
float accelX = 0, accelY = 0, accelZ = 0;
int currentMood = HAPPY;
float lastAccelMag = 1.0;

// ==========================================
// FEATURE TOGGLES
// ==========================================
struct Features {
  bool tracking = true;
  bool automood = true;
  bool buzzer = true;
  bool led = true;
  bool blinker = true;
  bool idle = false;
  bool sweat = false;
  bool curiosity = true;
  bool autoExpressions = true;
  bool touchEnabled = true;
  bool invertDisplay = false; // OLED color inversion
} features;

// ==========================================
// CALIBRAÇÃO
// ==========================================
float eyeThreshold = 0.3;
float shakeThreshold = 1.5;

// Eye shape settings (persistable)
int eyeW = 36, eyeH = 36, eyeR = 8, eyeS = 10;

// ==========================================
// DISPLAY MODE
// ==========================================
enum DisplayMode { MODE_EYES, MODE_MENU, MODE_ANIM };
DisplayMode displayMode = MODE_EYES;
int menuPage = 0;
const int MENU_PAGES = 5; // +1 para relógio
unsigned long menuStartTime = 0;
const long menuPageDuration = 3000;

// ==========================================
// ANIMATION STATE
// ==========================================
bool animRunning = false;
unsigned long animStartTime = 0;
int animType = 0;

// ==========================================
// FORWARD DECLARATIONS
// ==========================================
void startAnimation(int type);
void showMenu();
void saveSettings();
void loadSettings();

// ==========================================
// TOUCH SENSOR STATE
// ==========================================
bool touchActive = false;
unsigned long touchStartTime = 0;
unsigned long lastTouchTime = 0;
int touchCount = 0;
bool waitingDoubleTap = false;
unsigned long doubleTapWindow = 400; // ms

// ==========================================
// RANDOM EXPRESSION TIMER
// ==========================================
unsigned long lastRandomAnim = 0;
unsigned long randomAnimInterval = 20000; // 15-30s

// ==========================================
// CLOCK (synced from dashboard)
// ==========================================
struct Clock {
  int h = 0, m = 0, s = 0;
  int day = 13, month = 2, year = 2026;
  unsigned long lastSync = 0;
  bool synced = false;
} clockData;

// ==========================================
// SPLASH THEME (0=minimal, 1=matrix, 2=wave)
// ==========================================
int splashTheme = 0;

// ==========================================
// TIMERS
// ==========================================
unsigned long lastSensorRead = 0;
const long sensorInterval = 1000;
unsigned long lastMoodUpdate = 0;
const long moodInterval = 5000;
unsigned long lastClockTick = 0;

// ==========================================
// NVS — Persistent Settings
// ==========================================
void saveSettings() {
  prefs.begin("roboeyes", false);
  // Toggles
  prefs.putBool("tracking", features.tracking);
  prefs.putBool("automood", features.automood);
  prefs.putBool("buzzer", features.buzzer);
  prefs.putBool("led", features.led);
  prefs.putBool("blinker", features.blinker);
  prefs.putBool("idle", features.idle);
  prefs.putBool("sweat", features.sweat);
  prefs.putBool("curiosity", features.curiosity);
  prefs.putBool("autoExpr", features.autoExpressions);
  prefs.putBool("touch", features.touchEnabled);
  prefs.putBool("invert", features.invertDisplay);
  // Calibration
  prefs.putFloat("eyeTh", eyeThreshold);
  prefs.putFloat("shakeTh", shakeThreshold);
  // Eye shape
  prefs.putInt("eyeW", eyeW);
  prefs.putInt("eyeH", eyeH);
  prefs.putInt("eyeR", eyeR);
  prefs.putInt("eyeS", eyeS);
  // Splash
  prefs.putInt("splash", splashTheme);
  prefs.end();
  Serial.println("[NVS] Salvo!");
}

void loadSettings() {
  prefs.begin("roboeyes", true); // read-only
  // Toggles
  features.tracking = prefs.getBool("tracking", true);
  features.automood = prefs.getBool("automood", true);
  features.buzzer = prefs.getBool("buzzer", true);
  features.led = prefs.getBool("led", true);
  features.blinker = prefs.getBool("blinker", true);
  features.idle = prefs.getBool("idle", false);
  features.sweat = prefs.getBool("sweat", false);
  features.curiosity = prefs.getBool("curiosity", true);
  features.autoExpressions = prefs.getBool("autoExpr", true);
  features.touchEnabled = prefs.getBool("touch", true);
  features.invertDisplay = prefs.getBool("invert", false);
  // Calibration
  eyeThreshold = prefs.getFloat("eyeTh", 0.3);
  shakeThreshold = prefs.getFloat("shakeTh", 1.5);
  // Eye shape
  eyeW = prefs.getInt("eyeW", 36);
  eyeH = prefs.getInt("eyeH", 36);
  eyeR = prefs.getInt("eyeR", 8);
  eyeS = prefs.getInt("eyeS", 10);
  // Splash
  splashTheme = prefs.getInt("splash", 0);
  prefs.end();
  Serial.println("[NVS] Configurações carregadas!");
}

// ==========================================
// MEMORY REPORTER
// ==========================================
void reportMemory() {
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t sketchSize = ESP.getSketchSize();
  uint32_t freeSketch = ESP.getFreeSketchSpace();
  uint32_t totalSketch = sketchSize + freeSketch;
  Serial.println("\n[MEM] ═══════════════════════════");
  Serial.printf("[MEM] Heap livre: %u bytes (%.1f KB)\n", freeHeap,
                freeHeap / 1024.0);
  Serial.printf("[MEM] Flash: %u/%u KB (%.0f%%)\n", sketchSize / 1024,
                totalSketch / 1024, (sketchSize * 100.0) / totalSketch);
  if (freeHeap < 32768)
    Serial.println("[MEM] ⚠️  Heap abaixo de 32KB!");
  else
    Serial.println("[MEM] ✅ Memória OK");
  Serial.println("[MEM] ═══════════════════════════");
}

// ==========================================
// BMI160 I2C
// ==========================================
void bmi160WriteReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(BMI160_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

uint8_t bmi160ReadReg(uint8_t reg) {
  Wire.beginTransmission(BMI160_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)BMI160_ADDR, (uint8_t)1);
  return Wire.read();
}

bool bmi160ReadAccel(int16_t &ax, int16_t &ay, int16_t &az) {
  Wire.beginTransmission(BMI160_ADDR);
  Wire.write(0x12);
  Wire.endTransmission(false);
  if (Wire.requestFrom((uint8_t)BMI160_ADDR, (uint8_t)6) != 6)
    return false;
  ax = Wire.read() | (Wire.read() << 8);
  ay = Wire.read() | (Wire.read() << 8);
  az = Wire.read() | (Wire.read() << 8);
  return true;
}

bool bmi160Init() {
  bmi160WriteReg(BMI160_REG_CMD, BMI160_CMD_SOFT_RESET);
  delay(100);
  uint8_t chipId = bmi160ReadReg(BMI160_REG_CHIP_ID);
  Serial.printf("[BMI160] Chip ID: 0x%02X\n", chipId);
  if (chipId != BMI160_CHIP_ID_VAL)
    return false;
  bmi160WriteReg(BMI160_REG_CMD, BMI160_CMD_ACC_NORMAL);
  delay(50);
  bmi160WriteReg(BMI160_REG_CMD, BMI160_CMD_GYR_NORMAL);
  delay(100);
  return true;
}

// ==========================================
// LED RGB
// ==========================================
void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
  if (!features.led) {
    analogWrite(LED_R, 0);
    analogWrite(LED_G, 0);
    analogWrite(LED_B, 0);
    return;
  }
  analogWrite(LED_R, r);
  analogWrite(LED_G, g);
  analogWrite(LED_B, b);
}

void setMoodColor(int mood) {
  switch (mood) {
  case HAPPY:
    setLedColor(0, 255, 0);
    break;
  case ANGRY:
    setLedColor(255, 0, 0);
    break;
  case TIRED:
    setLedColor(0, 0, 255);
    break;
  default:
    setLedColor(255, 255, 255);
    break;
  }
}

void setRainbowLed(unsigned long elapsed) {
  int hue = (elapsed / 5) % 360;
  float c = 1.0, x = 1.0 - fabs(fmod(hue / 60.0, 2) - 1);
  float rf, gf, bf;
  if (hue < 60) {
    rf = c;
    gf = x;
    bf = 0;
  } else if (hue < 120) {
    rf = x;
    gf = c;
    bf = 0;
  } else if (hue < 180) {
    rf = 0;
    gf = c;
    bf = x;
  } else if (hue < 240) {
    rf = 0;
    gf = x;
    bf = c;
  } else if (hue < 300) {
    rf = x;
    gf = 0;
    bf = c;
  } else {
    rf = c;
    gf = 0;
    bf = x;
  }
  setLedColor(rf * 255, gf * 255, bf * 255);
}

// ==========================================
// BUZZER — Melodias
// ==========================================
void playTone(int freq, int dur) {
  if (!features.buzzer)
    return;
  tone(BUZZER_PIN, freq, dur);
}

void playMelodyBoot() {
  int notes[] = {523, 659, 784, 1047, 784, 1047}; // C5,E5,G5,C6,G5,C6
  int durs[] = {100, 100, 100, 150, 80, 200};
  for (int i = 0; i < 6; i++) {
    playTone(notes[i], durs[i]);
    delay(durs[i] + 30);
  }
}

void playMelodyTouch() { playTone(1200, 40); }

void playMelodyMenu() {
  playTone(800, 30);
  delay(50);
  playTone(1000, 30);
}

void playMelodyMoodChange(int mood) {
  switch (mood) {
  case HAPPY:
    playTone(1047, 80);
    delay(90);
    playTone(1319, 80);
    delay(90);
    playTone(1568, 120);
    break;
  case ANGRY:
    playTone(200, 150);
    delay(60);
    playTone(150, 200);
    break;
  case TIRED:
    playTone(523, 150);
    delay(100);
    playTone(392, 200);
    break;
  default:
    playTone(880, 60);
    break;
  }
}

void playMelodyExpression(int type) {
  switch (type) {
  case 0:
    playTone(880, 60);
    delay(70);
    playTone(1100, 60);
    delay(70);
    playTone(1320, 100);
    break; // love
  case 1:
    playTone(2000, 40);
    delay(50);
    playTone(2500, 60);
    break; // scared
  case 2:
    playTone(600, 80);
    delay(100);
    playTone(500, 80);
    break; // suspicious
  case 3:
    playTone(400, 200);
    break; // sleepy
  case 4:
    playTone(1500, 40);
    delay(50);
    playTone(1800, 40);
    delay(50);
    playTone(2200, 60);
    break; // excited
  case 5:
    playTone(700, 60);
    delay(60);
    playTone(500, 60);
    delay(60);
    playTone(700, 60);
    break; // dizzy
  }
}

// ==========================================
// CLOCK — Relógio interno
// ==========================================
void tickClock() {
  if (!clockData.synced)
    return;
  unsigned long now = millis();
  if (now - lastClockTick >= 1000) {
    lastClockTick = now;
    clockData.s++;
    if (clockData.s >= 60) {
      clockData.s = 0;
      clockData.m++;
    }
    if (clockData.m >= 60) {
      clockData.m = 0;
      clockData.h++;
    }
    if (clockData.h >= 24) {
      clockData.h = 0;
    }
  }
}

// ==========================================
// ACELERÔMETRO → OLHOS
// ==========================================
void updateEyePosition() {
  if (!bmi160Ok || !features.tracking)
    return;
  int16_t rawX, rawY, rawZ;
  if (!bmi160ReadAccel(rawX, rawY, rawZ))
    return;

  accelX = rawX / 16384.0;
  accelY = rawY / 16384.0;
  accelZ = rawZ / 16384.0;

  float mag = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);
  float delta = abs(mag - lastAccelMag);
  lastAccelMag = mag;

  if (delta > shakeThreshold + 0.5) {
    startAnimation(5);
    return;
  } // dizzy
  if (delta > shakeThreshold) {
    startAnimation(1);
    return;
  } // scared

  float th = eyeThreshold;
  if (accelX < -th && accelY < -th)
    roboEyes.setPosition(NW);
  else if (accelX > th && accelY < -th)
    roboEyes.setPosition(NE);
  else if (accelX < -th && accelY > th)
    roboEyes.setPosition(SW);
  else if (accelX > th && accelY > th)
    roboEyes.setPosition(SE);
  else if (accelX < -th)
    roboEyes.setPosition(W);
  else if (accelX > th)
    roboEyes.setPosition(E);
  else if (accelY < -th)
    roboEyes.setPosition(N);
  else if (accelY > th)
    roboEyes.setPosition(S);
  else
    roboEyes.setPosition(DEFAULT);
}

// ==========================================
// TEMPERATURA → HUMOR
// ==========================================
void updateMood() {
  if (!htuOk || !features.automood)
    return;
  int newMood;
  if (currentTemp > 30.0) {
    newMood = ANGRY;
    if (features.sweat)
      roboEyes.setSweat(ON);
  } else if (currentTemp < 18.0) {
    newMood = TIRED;
    roboEyes.setSweat(OFF);
  } else {
    newMood = HAPPY;
    roboEyes.setSweat(OFF);
  }
  if (newMood != currentMood) {
    currentMood = newMood;
    roboEyes.setMood(currentMood);
    setMoodColor(currentMood);
    playMelodyMoodChange(currentMood);
  }
}

void readSensors() {
  if (htuOk) {
    float t = htu.readTemperature();
    float h = htu.readHumidity();
    if (!isnan(t))
      currentTemp = t;
    if (!isnan(h))
      currentHum = h;
  }
}

// ==========================================
// TOUCH SENSOR — Gestos
// ==========================================
void handleTouch() {
  if (!features.touchEnabled)
    return;
  int val = touchRead(TOUCH_PIN);
  bool touching = (val < TOUCH_THRESHOLD);
  unsigned long now = millis();

  // Detecta início do toque
  if (touching && !touchActive) {
    touchActive = true;
    touchStartTime = now;
  }

  // Detecta fim do toque
  if (!touching && touchActive) {
    touchActive = false;
    unsigned long duration = now - touchStartTime;

    if (duration > 1000) {
      // TOQUE LONGO → Menu
      Serial.println("[TOUCH] Longo -> Menu");
      showMenu();
    } else if (duration > 50) {
      // Toque curto — verifica duplo
      if (waitingDoubleTap && (now - lastTouchTime < doubleTapWindow)) {
        // TOQUE DUPLO → Cicla humor
        waitingDoubleTap = false;
        int moods[] = {HAPPY, ANGRY, TIRED, DEFAULT};
        int idx = 0;
        for (int i = 0; i < 4; i++) {
          if (moods[i] == currentMood) {
            idx = (i + 1) % 4;
            break;
          }
        }
        currentMood = moods[idx];
        roboEyes.setMood(currentMood);
        setMoodColor(currentMood);
        playMelodyMoodChange(currentMood);
        Serial.printf("[TOUCH] Duplo -> Mood %d\n", currentMood);
      } else {
        waitingDoubleTap = true;
        lastTouchTime = now;
      }
    }
  }

  // Timeout de espera de duplo toque → executa toque simples
  if (waitingDoubleTap && !touchActive &&
      (now - lastTouchTime > doubleTapWindow)) {
    waitingDoubleTap = false;
    // TOQUE SIMPLES → Expressão aleatória
    int randAnim = random(0, 12);
    if (randAnim < 6) {
      startAnimation(randAnim);
    } else {
      // Animações built-in do RoboEyes
      switch (randAnim) {
      case 6:
        roboEyes.blink();
        break;
      case 7:
        roboEyes.anim_confused();
        break;
      case 8:
        roboEyes.anim_laugh();
        break;
      case 9:
        roboEyes.blink(1, 0);
        break;
      case 10:
        roboEyes.blink(0, 1);
        break;
      case 11: {
        static bool cyc = false;
        cyc = !cyc;
        roboEyes.setCyclops(cyc ? ON : OFF);
        break;
      }
      }
    }
    playMelodyTouch();
    Serial.printf("[TOUCH] Simples -> Anim %d\n", randAnim);
  }
}

// ==========================================
// RANDOM EXPRESSIONS TIMER
// ==========================================
void updateRandomExpressions() {
  if (!features.autoExpressions || displayMode != MODE_EYES)
    return;
  unsigned long now = millis();
  if (now - lastRandomAnim >= randomAnimInterval) {
    lastRandomAnim = now;
    randomAnimInterval = random(15000, 30000); // Próximo intervalo aleatório
    int randAnim = random(0, 12);
    if (randAnim < 6) {
      startAnimation(randAnim);
    } else {
      switch (randAnim) {
      case 6:
        roboEyes.blink();
        break;
      case 7:
        roboEyes.anim_confused();
        break;
      case 8:
        roboEyes.anim_laugh();
        break;
      case 9:
        roboEyes.blink(1, 0);
        break;
      case 10:
        roboEyes.blink(0, 1);
        break;
      case 11:
        roboEyes.blink();
        break;
      }
    }
    Serial.printf("[RAND] Anim %d (next in %lus)\n", randAnim,
                  randomAnimInterval / 1000);
  }
}

// ==========================================
// MENU OLED — 5 TELAS
// ==========================================
void drawMenuHeader(const char *icon, const char *title) {
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf(" %s %s", icon, title);
  display.drawLine(0, 12, 127, 12, SSD1306_WHITE);
  // Page indicator
  for (int i = 0; i < MENU_PAGES; i++) {
    if (i == menuPage)
      display.fillCircle(54 + i * 8, 62, 2, SSD1306_WHITE);
    else
      display.drawCircle(54 + i * 8, 62, 2, SSD1306_WHITE);
  }
}

void drawWifiScreen() {
  display.clearDisplay();
  drawMenuHeader("\xF0", "WiFi AP");
  display.setCursor(0, 18);
  display.printf("SSID: %s\n", AP_SSID);
  display.printf("Senha: %s\n\n", AP_PASS);
  display.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
  display.printf("Clientes: %d", WiFi.softAPgetStationNum());
  display.display();
}

void drawSensorScreen() {
  display.clearDisplay();
  drawMenuHeader("\xB0", "Sensores");
  display.setCursor(0, 18);
  display.printf("Temp: %.1f C  Hum: %.0f%%\n\n", currentTemp, currentHum);
  display.printf("Acc X: %+.2f\n", accelX);
  display.printf("Acc Y: %+.2f\n", accelY);
  display.printf("Acc Z: %+.2f\n", accelZ);
  const char *moods[] = {"DEFAULT", "HAPPY", "TIRED", "ANGRY"};
  int mi = (currentMood >= 0 && currentMood <= 3) ? currentMood : 0;
  display.printf("Humor: %s", moods[mi]);
  display.display();
}

void drawClockScreen() {
  display.clearDisplay();
  drawMenuHeader("\xB7", "Relogio");
  if (!clockData.synced) {
    display.setCursor(10, 28);
    display.setTextSize(1);
    display.println("Conecte ao Dashboard");
    display.setCursor(10, 40);
    display.println("para sincronizar!");
  } else {
    display.setTextSize(2);
    display.setCursor(16, 20);
    display.printf("%02d:%02d:%02d", clockData.h, clockData.m, clockData.s);
    display.setTextSize(1);
    display.setCursor(25, 44);
    display.printf("%02d/%02d/%04d", clockData.day, clockData.month,
                   clockData.year);
  }
  display.display();
}

void drawSystemScreen() {
  display.clearDisplay();
  drawMenuHeader("\xDB", "Sistema");
  unsigned long upSec = millis() / 1000;
  display.setCursor(0, 18);
  display.printf("Uptime: %02lu:%02lu:%02lu\n\n", upSec / 3600,
                 (upSec % 3600) / 60, upSec % 60);
  display.printf("RAM livre: %d B\n", ESP.getFreeHeap());
  display.printf("Flash: %d KB\n", ESP.getSketchSize() / 1024);
  display.printf("BMI160: %s  HTU: %s\n", bmi160Ok ? "OK" : "--",
                 htuOk ? "OK" : "--");
  display.printf("Touch: %d", touchRead(TOUCH_PIN));
  display.display();
}

void drawAboutScreen() {
  display.clearDisplay();
  drawMenuHeader("\x03", "Sobre");
  display.setCursor(0, 20);
  display.println("  RoboEyes v5.0");
  display.println("");
  display.println("  ESP32-WROOM HW-724");
  display.println("  OLED 128x64 bicolor");
  display.println("");
  display.println("  github.com/raunick");
  display.display();
}

void showMenu() {
  displayMode = MODE_MENU;
  menuPage = 0;
  menuStartTime = millis();
  playMelodyMenu();
}

void updateMenu() {
  unsigned long elapsed = millis() - menuStartTime;
  int page = (elapsed / menuPageDuration);

  if (page >= MENU_PAGES) {
    displayMode = MODE_EYES;
    playTone(1200, 50);
    return;
  }

  if (page != menuPage) {
    menuPage = page;
    playTone(600, 20);
  }

  // Redraw current page (for clock updates)
  switch (menuPage) {
  case 0:
    drawWifiScreen();
    break;
  case 1:
    drawSensorScreen();
    break;
  case 2:
    drawClockScreen();
    break;
  case 3:
    drawSystemScreen();
    break;
  case 4:
    drawAboutScreen();
    break;
  }
}

// ==========================================
// SPLASH SCREENS
// ==========================================
void splashMinimal() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 5);
  display.println("RoboEyes");
  display.setTextSize(1);
  display.setCursor(35, 30);
  display.println("v5.0 Ready!");
  display.setCursor(20, 48);
  display.printf("WiFi: %s", AP_SSID);
  display.display();
}

void splashMatrix() {
  for (int frame = 0; frame < 20; frame++) {
    display.clearDisplay();
    for (int col = 0; col < 128; col += 6) {
      int y = random(0, 64);
      for (int row = y; row < min(y + 20, 64); row += 8) {
        display.setCursor(col, row);
        display.print((char)random(33, 126));
      }
    }
    if (frame > 10) {
      display.setTextSize(2);
      display.setCursor(10, 20);
      display.print("RoboEyes");
      display.setTextSize(1);
      display.setCursor(40, 45);
      display.print("v5.0");
    }
    display.display();
    delay(80);
  }
}

void splashWave() {
  for (int frame = 0; frame < 30; frame++) {
    display.clearDisplay();
    for (int x = 0; x < 128; x++) {
      int y = 32 + (int)(15.0 * sin((x + frame * 8) * 0.05));
      int h = map(frame, 0, 29, 2, 32);
      display.drawLine(x, y - h / 2, x, y + h / 2, SSD1306_WHITE);
    }
    if (frame > 15) {
      display.setTextColor(SSD1306_BLACK);
      display.setTextSize(2);
      display.setCursor(10, 20);
      display.print("RoboEyes");
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(40, 45);
      display.print("v5.0");
    }
    display.display();
    delay(50);
  }
}

void playSplash() {
  switch (splashTheme) {
  case 0:
    splashMinimal();
    delay(2000);
    break;
  case 1:
    splashMatrix();
    break;
  case 2:
    splashWave();
    break;
  }
}

// ==========================================
// ANIMAÇÕES (6 expressões)
// ==========================================
void startAnimation(int type) {
  animType = type;
  animStartTime = millis();
  animRunning = true;
  displayMode = MODE_ANIM;
  playMelodyExpression(type);
  Serial.printf("[ANIM] Tipo %d\n", type);
}

void restoreEyes() {
  animRunning = false;
  displayMode = MODE_EYES;
  roboEyes.setWidth(eyeW, eyeW);
  roboEyes.setHeight(eyeH, eyeH);
  roboEyes.setBorderradius(eyeR, eyeR);
  roboEyes.setSpacebetween(eyeS);
  roboEyes.setMood(currentMood);
  setMoodColor(currentMood);
}

void updateAnimLove() {
  unsigned long e = millis() - animStartTime;
  if (e > 2000) {
    restoreEyes();
    return;
  }
  setLedColor(255, 50, 150);
  int f = (e / 150) % 2;
  roboEyes.setHeight(f ? 10 : 30, f ? 10 : 30);
  roboEyes.setBorderradius(f ? 5 : 15, f ? 5 : 15);
  roboEyes.setSpacebetween(5);
  roboEyes.setMood(HAPPY);
  roboEyes.update();
}

void updateAnimScared() {
  unsigned long e = millis() - animStartTime;
  if (e > 1500) {
    restoreEyes();
    return;
  }
  setLedColor(255, 255, 0);
  roboEyes.setWidth(50, 50);
  roboEyes.setHeight(50, 50);
  roboEyes.setBorderradius(25, 25);
  roboEyes.setSpacebetween(15);
  roboEyes.setPosition((e / 60) % 2 ? N : S);
  roboEyes.update();
}

void updateAnimSuspicious() {
  unsigned long e = millis() - animStartTime;
  if (e > 3000) {
    restoreEyes();
    return;
  }
  setLedColor(255, 165, 0);
  roboEyes.setHeight(36, 15);
  int positions[] = {E, E, DEFAULT, W, W, DEFAULT};
  roboEyes.setPosition(positions[(e / 500) % 6]);
  roboEyes.update();
}

void updateAnimSleepy() {
  unsigned long e = millis() - animStartTime;
  if (e > 2500) {
    restoreEyes();
    return;
  }
  setLedColor(100, 100, 255);
  roboEyes.setMood(TIRED);
  int h = max(4, (int)(36 - (e * 30 / 2500)));
  roboEyes.setHeight(h, h);
  roboEyes.setPosition(S);
  roboEyes.update();
}

void updateAnimExcited() {
  unsigned long e = millis() - animStartTime;
  if (e > 2000) {
    restoreEyes();
    return;
  }
  setRainbowLed(e);
  roboEyes.setWidth(45, 45);
  roboEyes.setHeight(45, 45);
  roboEyes.setBorderradius(4, 4);
  roboEyes.setMood(HAPPY);
  roboEyes.setPosition((e / 100) % 2 ? N : S);
  roboEyes.update();
}

void updateAnimDizzy() {
  unsigned long e = millis() - animStartTime;
  if (e > 2500) {
    restoreEyes();
    roboEyes.setSweat(OFF);
    return;
  }
  setLedColor(200, 50, 200);
  roboEyes.setSweat(ON);
  int circ[] = {N, NE, E, SE, S, SW, W, NW};
  roboEyes.setPosition(circ[(e / 180) % 8]);
  roboEyes.update();
}

void updateAnimation() {
  switch (animType) {
  case 0:
    updateAnimLove();
    break;
  case 1:
    updateAnimScared();
    break;
  case 2:
    updateAnimSuspicious();
    break;
  case 3:
    updateAnimSleepy();
    break;
  case 4:
    updateAnimExcited();
    break;
  case 5:
    updateAnimDizzy();
    break;
  default:
    restoreEyes();
    break;
  }
}

// ==========================================
// WEB SERVER
// ==========================================
void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r) {
    r->send(200, "text/html", DASHBOARD_HTML);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *r) {
    JsonDocument doc;
    doc["temp"] = currentTemp;
    doc["hum"] = currentHum;
    doc["ax"] = accelX;
    doc["ay"] = accelY;
    doc["az"] = accelZ;
    doc["mood"] = currentMood;
    doc["bmi"] = bmi160Ok;
    doc["htu"] = htuOk;
    doc["mode"] = (int)displayMode;
    doc["touch"] = touchRead(TOUCH_PIN);
    doc["clockSynced"] = clockData.synced;
    if (clockData.synced) {
      doc["ch"] = clockData.h;
      doc["cm"] = clockData.m;
      doc["cs"] = clockData.s;
    }
    // Memory info
    doc["heap"] = ESP.getFreeHeap();
    doc["flash"] = ESP.getSketchSize();
    JsonObject t = doc["t"].to<JsonObject>();
    t["tracking"] = features.tracking;
    t["automood"] = features.automood;
    t["buzzer"] = features.buzzer;
    t["led"] = features.led;
    t["blinker"] = features.blinker;
    t["idle"] = features.idle;
    t["sweat"] = features.sweat;
    t["curiosity"] = features.curiosity;
    t["autoExpr"] = features.autoExpressions;
    t["touch"] = features.touchEnabled;
    t["invert"] = features.invertDisplay;
    String json;
    serializeJson(doc, json);
    r->send(200, "application/json", json);
  });

  server.on(
      "/api/toggle", HTTP_POST, [](AsyncWebServerRequest *r) {}, NULL,
      [](AsyncWebServerRequest *r, uint8_t *data, size_t len, size_t idx,
         size_t tot) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len)) {
          r->send(400);
          return;
        }
        String f = doc["feature"].as<String>();
        bool s = doc["state"];
        if (f == "tracking")
          features.tracking = s;
        else if (f == "automood")
          features.automood = s;
        else if (f == "buzzer")
          features.buzzer = s;
        else if (f == "led") {
          features.led = s;
          if (!s) {
            analogWrite(LED_R, 0);
            analogWrite(LED_G, 0);
            analogWrite(LED_B, 0);
          } else
            setMoodColor(currentMood);
        } else if (f == "blinker") {
          features.blinker = s;
          roboEyes.setAutoblinker(s ? ON : OFF, 3, 2);
        } else if (f == "idle") {
          features.idle = s;
          roboEyes.setIdleMode(s ? ON : OFF, 2, 2);
        } else if (f == "sweat") {
          features.sweat = s;
          roboEyes.setSweat(s ? ON : OFF);
        } else if (f == "curiosity") {
          features.curiosity = s;
          roboEyes.setCuriosity(s ? ON : OFF);
        } else if (f == "autoExpr")
          features.autoExpressions = s;
        else if (f == "touch")
          features.touchEnabled = s;
        else if (f == "invert") {
          features.invertDisplay = s;
          display.invertDisplay(s);
        }
        Serial.printf("[TOGGLE] %s=%s\n", f.c_str(), s ? "ON" : "OFF");
        saveSettings();
        r->send(200, "application/json", "{\"ok\":true}");
      });

  server.on(
      "/api/mood", HTTP_POST, [](AsyncWebServerRequest *r) {}, NULL,
      [](AsyncWebServerRequest *r, uint8_t *data, size_t len, size_t idx,
         size_t tot) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len)) {
          r->send(400);
          return;
        }
        String m = doc["mood"].as<String>();
        if (m == "happy")
          currentMood = HAPPY;
        else if (m == "angry")
          currentMood = ANGRY;
        else if (m == "tired")
          currentMood = TIRED;
        else
          currentMood = DEFAULT;
        roboEyes.setMood(currentMood);
        setMoodColor(currentMood);
        playMelodyMoodChange(currentMood);
        r->send(200, "application/json", "{\"ok\":true}");
      });

  server.on(
      "/api/eyes", HTTP_POST, [](AsyncWebServerRequest *r) {}, NULL,
      [](AsyncWebServerRequest *r, uint8_t *data, size_t len, size_t idx,
         size_t tot) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len)) {
          r->send(400);
          return;
        }
        String a = doc["action"].as<String>();
        if (a == "blink")
          roboEyes.blink();
        else if (a == "confused")
          roboEyes.anim_confused();
        else if (a == "laugh")
          roboEyes.anim_laugh();
        else if (a == "wink_l")
          roboEyes.blink(1, 0);
        else if (a == "wink_r")
          roboEyes.blink(0, 1);
        else if (a == "cyclops") {
          static bool c = false;
          c = !c;
          roboEyes.setCyclops(c ? ON : OFF);
        } else if (a == "love")
          startAnimation(0);
        else if (a == "scared")
          startAnimation(1);
        else if (a == "suspicious")
          startAnimation(2);
        else if (a == "sleepy")
          startAnimation(3);
        else if (a == "excited")
          startAnimation(4);
        else if (a == "dizzy")
          startAnimation(5);
        playTone(1200, 50);
        r->send(200, "application/json", "{\"ok\":true}");
      });

  server.on(
      "/api/calibrate", HTTP_POST, [](AsyncWebServerRequest *r) {}, NULL,
      [](AsyncWebServerRequest *r, uint8_t *data, size_t len, size_t idx,
         size_t tot) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len)) {
          r->send(400);
          return;
        }
        if (doc["threshold"])
          eyeThreshold = doc["threshold"];
        if (doc["shakeThreshold"])
          shakeThreshold = doc["shakeThreshold"];
        saveSettings();
        r->send(200, "application/json", "{\"ok\":true}");
      });

  server.on(
      "/api/shape", HTTP_POST, [](AsyncWebServerRequest *r) {}, NULL,
      [](AsyncWebServerRequest *r, uint8_t *data, size_t len, size_t idx,
         size_t tot) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len)) {
          r->send(400);
          return;
        }
        if (doc["w"]) {
          eyeW = doc["w"];
          roboEyes.setWidth(eyeW, eyeW);
        }
        if (doc["h"]) {
          eyeH = doc["h"];
          roboEyes.setHeight(eyeH, eyeH);
        }
        if (doc["r"]) {
          eyeR = doc["r"];
          roboEyes.setBorderradius(eyeR, eyeR);
        }
        if (doc["s"].is<int>()) {
          eyeS = doc["s"];
          roboEyes.setSpacebetween(eyeS);
        }
        saveSettings();
        r->send(200, "application/json", "{\"ok\":true}");
      });

  server.on(
      "/api/screen", HTTP_POST, [](AsyncWebServerRequest *r) {}, NULL,
      [](AsyncWebServerRequest *r, uint8_t *data, size_t len, size_t idx,
         size_t tot) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len)) {
          r->send(400);
          return;
        }
        String a = doc["action"].as<String>();
        if (a == "menu")
          showMenu();
        else if (a == "eyes")
          displayMode = MODE_EYES;
        r->send(200, "application/json", "{\"ok\":true}");
      });

  // Sincronizar relógio
  server.on(
      "/api/time", HTTP_POST, [](AsyncWebServerRequest *r) {}, NULL,
      [](AsyncWebServerRequest *r, uint8_t *data, size_t len, size_t idx,
         size_t tot) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len)) {
          r->send(400);
          return;
        }
        clockData.h = doc["h"];
        clockData.m = doc["m"];
        clockData.s = doc["s"];
        clockData.day = doc["d"];
        clockData.month = doc["mo"];
        clockData.year = doc["y"];
        clockData.synced = true;
        clockData.lastSync = millis();
        lastClockTick = millis();
        Serial.printf("[CLOCK] %02d:%02d:%02d %02d/%02d/%04d\n", clockData.h,
                      clockData.m, clockData.s, clockData.day, clockData.month,
                      clockData.year);
        r->send(200, "application/json", "{\"ok\":true}");
      });

  // Mudar tema splash
  server.on(
      "/api/splash", HTTP_POST, [](AsyncWebServerRequest *r) {}, NULL,
      [](AsyncWebServerRequest *r, uint8_t *data, size_t len, size_t idx,
         size_t tot) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len)) {
          r->send(400);
          return;
        }
        splashTheme = doc["theme"] | 0;
        Serial.printf("[SPLASH] Tema %d\n", splashTheme);
        saveSettings();
        r->send(200, "application/json", "{\"ok\":true}");
      });

  server.begin();
  Serial.println("[OK] Web Server em 192.168.4.1");
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== RoboEyes Enhanced v5.1 ===");

  // Carregar configurações salvas da NVS
  loadSettings();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  setLedColor(255, 255, 255);

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("[OK] WiFi AP: %s (%s)\n", AP_SSID,
                WiFi.softAPIP().toString().c_str());

  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[ERRO] OLED falhou!");
    for (;;)
      ;
  }
  Serial.println("[OK] OLED");
  display.clearDisplay();
  display.display();

  htuOk = htu.begin();
  Serial.printf("[%s] HTU21D\n", htuOk ? "OK" : "--");

  bmi160Ok = bmi160Init();
  Serial.printf("[%s] BMI160\n", bmi160Ok ? "OK" : "--");

  // Touch
  Serial.printf("[OK] Touch (GPIO%d) val=%d\n", TOUCH_PIN,
                touchRead(TOUCH_PIN));

  // RoboEyes — aplica shape salvo
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  roboEyes.setWidth(eyeW, eyeW);
  roboEyes.setHeight(eyeH, eyeH);
  roboEyes.setBorderradius(eyeR, eyeR);
  roboEyes.setSpacebetween(eyeS);
  roboEyes.setMood(HAPPY);
  setMoodColor(HAPPY);

  if (!bmi160Ok) {
    features.tracking = false;
    roboEyes.setIdleMode(ON, 2, 2);
    features.idle = true;
  }
  // Aplica toggles salvos
  roboEyes.setAutoblinker(features.blinker ? ON : OFF, 3, 2);
  roboEyes.setCuriosity(features.curiosity ? ON : OFF);
  roboEyes.setIdleMode(features.idle ? ON : OFF, 2, 2);
  roboEyes.setSweat(features.sweat ? ON : OFF);
  if (!features.led) {
    analogWrite(LED_R, 0);
    analogWrite(LED_G, 0);
    analogWrite(LED_B, 0);
  }

  setupWebServer();

  // Splash + Boot melody
  playMelodyBoot();
  playSplash();
  setLedColor(0, 255, 0);

  Serial.printf("\n[PRONTO] BMI160:%s HTU21D:%s\n", bmi160Ok ? "SIM" : "NAO",
                htuOk ? "SIM" : "NAO");
  Serial.printf("[WIFI] '%s' senha '%s'\n", AP_SSID, AP_PASS);
  Serial.println("[WEB] http://192.168.4.1");
  Serial.println("[TOUCH] GPIO15 ativo");

  // Aplica inversão salva
  display.invertDisplay(features.invertDisplay);

  // Relatório de memória
  reportMemory();

  lastRandomAnim = millis();
  randomSeed(analogRead(36));
}

// ==========================================
// LOOP
// ==========================================
void loop() {
  unsigned long now = millis();

  // Sempre ativos
  handleTouch();
  tickClock();

  if (now - lastSensorRead >= sensorInterval) {
    lastSensorRead = now;
    readSensors();
  }

  switch (displayMode) {
  case MODE_EYES:
    if (features.tracking && bmi160Ok)
      updateEyePosition();
    if (now - lastMoodUpdate >= moodInterval) {
      lastMoodUpdate = now;
      updateMood();
    }
    updateRandomExpressions();
    roboEyes.update();
    break;

  case MODE_MENU:
    updateMenu();
    break;

  case MODE_ANIM:
    if (animRunning)
      updateAnimation();
    else
      restoreEyes();
    break;
  }
}
