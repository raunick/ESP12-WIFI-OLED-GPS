#include "Adafruit_HTU21DF.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Wire.h>
#include <math.h>
#include <time.h> // Nativo do ESP32/C++

// --- Network & Web ---
#include "web_assets.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

// ==========================================
// CONFIGURAÇÕES WIFI
// ==========================================
const char *ssid = "RAUL";
const char *password = "24681357";
// ==========================================

// --- Hardware Setup ---
#define I2C_SDA 5
#define I2C_SCL 4
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define YELLOW_HEIGHT 16

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
AsyncWebServer server(80);

// Configuração de Tempo (Brasília UTC-3)
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 3600;
const int daylightOffset_sec = 0;

// --- Variáveis Globais ---
unsigned long lastUpdate = 0;
const long updateInterval = 2000;
unsigned long lastAnimUpdate = 0;
const long animInterval = 200;
int animationFrame = 0;
bool sensorOk = true;

// --- BITMAPS ---

// WiFi Conectado (12x10) - Ondas clássicas
const unsigned char PROGMEM icon_wifi_on[] = {
    0x1F, 0x80, 0x7F, 0xE0, 0xE0, 0x70, 0x0F, 0x00, 0x3F, 0xC0, 
    0x30, 0xC0, 0x06, 0x00, 0x0F, 0x00, 0x06, 0x00, 0x06, 0x00
};

// WiFi Desconectado (12x10) - Com um "X" de erro
const unsigned char PROGMEM icon_wifi_off[] = {
    0x1F, 0x80, 0x10, 0x80, 0x12, 0x40, 0x04, 0x80, 0x01, 0x00, 
    0x02, 0x80, 0x04, 0x40, 0x02, 0x80, 0x01, 0x00, 0x00, 0x00
};
// Lua realista (20x20) - Com crateras e textura
const unsigned char PROGMEM icon_moon[] = {
0x07, 0x80, 0x00, //     ####              (Estrela topo)
    0x0F, 0xC0, 0x10, //    ######         .
    0x1F, 0x00, 0x00, //   #####
    0x3E, 0x00, 0x08, //  ####             .
    0x7C, 0x00, 0x00, // #####
    0x78, 0x04, 0x00, // ####         .
    0xF0, 0x00, 0x20, //####              .
    0xF0, 0x00, 0x00, //####
    0xF0, 0x02, 0x00, //####        .
    0xF0, 0x00, 0x00, //####
    0xF0, 0x00, 0x00, //####
    0x78, 0x01, 0x00, // ####       .
    0x7C, 0x00, 0x00, // #####
    0x3E, 0x00, 0x10, //  ####             .
    0x1F, 0x00, 0x00, //   #####
    0x0F, 0xC0, 0x00, //    ######
    0x07, 0x80, 0x04, //     ####         .
    0x01, 0xE0, 0x00, //       ####
    0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00
};
// Gota Outline (12x14)
const unsigned char PROGMEM icon_drop_outline[] = {
    0x06, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x1F, 0x80, 0x3F, 0xC0, 
    0x7F, 0xE0, 0x70, 0xE0, 0xE0, 0x70, 0xE0, 0x70, 0xE0, 0x70, 
    0x70, 0xE0, 0x3F, 0xC0, 0x1F, 0x80, 0x00, 0x00
};
// --- Funções Auxiliares ---

// Desenha Sol (Dia)
void drawAnimatedSun(int x, int y, int frame) {
  int cx = x + 10;
  int cy = y + 10;
  int radius = 4;
  display.fillCircle(cx, cy, radius, SSD1306_WHITE);
  float angleOffset = (frame * (PI / 2.0));
  for (int i = 0; i < 8; i++) {
    float angle = (i * PI / 4) + angleOffset;
    display.drawLine(cx + 6 * cos(angle), cy + 6 * sin(angle),
                     cx + 10 * cos(angle), cy + 10 * sin(angle), SSD1306_WHITE);
  }
}

// Desenha Gota Animada
void drawAnimatedDrop(int x, int y, float humidity) {
  int maxFillHeight = 10;
  int fillHeight = (humidity / 100.0) * maxFillHeight;
  if (fillHeight < 1)
    fillHeight = 1;
  display.fillRect(x + 2, y + (14 - fillHeight) - 2, 6, fillHeight,
                   SSD1306_WHITE);
  display.drawBitmap(x, y, icon_drop_outline, 10, 14, SSD1306_WHITE);
}

// Verifica se é Dia ou Noite
bool isDaytime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return true; // Default dia se falhar
  }
  // Considera dia entre 06:00 e 18:00
  return (timeinfo.tm_hour >= 6 && timeinfo.tm_hour < 18);
}

void drawHeader() {
  // Área Amarela: Hora + Data + WiFi
  struct tm timeinfo;
  bool timeSynced = getLocalTime(&timeinfo);

  display.setFont();
  display.setTextSize(1);

  if (timeSynced) {
    // 1. Hora (Esquerda) -> "20:25"
    display.setCursor(0, 4);
    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    display.print(timeStr);

    // 2. Data + Dia da Semana (Centro) -> "08/02 SAB"
    display.setCursor(45, 4);
    char dateStr[12];
    const char *days[] = {"DOM", "SEG", "TER", "QUA", "QUI", "SEX", "SAB"};
    sprintf(dateStr, "%02d/%02d %s", timeinfo.tm_mday, timeinfo.tm_mon + 1,
            days[timeinfo.tm_wday]);
    display.print(dateStr);
  } else {
    display.setCursor(0, 4);
    display.print("--:-- --/-- ---");
  }

  // 3. WiFi (Direita)
  if (WiFi.status() == WL_CONNECTED) {
    display.drawBitmap(116, 3, icon_wifi_on, 12, 10, SSD1306_WHITE);
  } else {
    display.drawBitmap(116, 3, icon_wifi_off, 12, 10, SSD1306_WHITE);
  }

  // Divisória Horizontal
  display.drawLine(0, YELLOW_HEIGHT, SCREEN_WIDTH, YELLOW_HEIGHT,
                   SSD1306_WHITE);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha OLED"));
    while (1)
      ;
  }
  display.setTextColor(SSD1306_WHITE);

  sensorOk = htu.begin();
  if (!sensorOk)
    Serial.println("HTU21D Falhou");

  // Conexão WiFi e Tempo
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  display.clearDisplay();
  display.setCursor(10, 30);
  display.print("Conectando...");
  display.display();

  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 20) {
    delay(500);
    t++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  }

  // Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  });
  server.on("/events", HTTP_GET, [](AsyncWebServerRequest *request) {
    float t = htu.readTemperature();
    float h = htu.readHumidity();
    if (isnan(t))
      t = 0;
    if (isnan(h))
      h = 0;
    String json =
        "{\"temperature\":" + String(t) + ",\"humidity\":" + String(h) + "}";
    request->send(200, "application/json", json);
  });
  server.begin();
}

void loop() {
  unsigned long currentMillis = millis();

  // Animação & Atualização de Tela
  if (currentMillis - lastAnimUpdate >= animInterval) {
    lastAnimUpdate = currentMillis;
    animationFrame = (animationFrame + 1) % 4;

    // Leitura rápida (HTU21D é rápido o suficiente)
    float t = htu.readTemperature();
    float h = htu.readHumidity();
    if (isnan(t))
      t = 0.0;
    if (isnan(h))
      h = 0.0;

    display.clearDisplay();

    // === TOPO (AMARELO) ===
    drawHeader();

    // === CORPO (AZUL) ===
    // 1. Ícone Clima (Sol/Lua)
    if (isDaytime()) {
      drawAnimatedSun(6, 22, animationFrame);
      display.setFont();
      display.setTextSize(1);
      display.setCursor(8, 48);
      display.print("DIA");
    } else {
      display.drawBitmap(6, 22, icon_moon, 20, 20, SSD1306_WHITE);
      display.setFont();
      display.setTextSize(1);
      display.setCursor(4, 48);
      display.print("NOITE");
    }

    // Divisória Vertical
    for (int i = YELLOW_HEIGHT + 2; i < 64; i += 3)
      display.drawPixel(56, i, SSD1306_WHITE);

    // 2. Valores
    // Temp
    display.setFont();
    display.setCursor(60, 18);
    display.print("temp");

    // Mostra Hora Grande ao lado da temp? Não, mostra Temp Grande
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(60, 44);
    display.print(t, 1);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(110, 38);
    display.print("C");
    display.drawCircle(107, 28, 2, SSD1306_WHITE);

    // Divisória Horizontal Direita
    for (int i = 60; i < 128; i += 3)
      display.drawPixel(i, 47, SSD1306_WHITE);

    // Hum
    display.setFont();
    display.setCursor(60, 52);
    display.print("umid");
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(90, 62);
    display.print((int)h);
    display.print("%");

    display.display();
  }
}