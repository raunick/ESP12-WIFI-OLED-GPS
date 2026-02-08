#include "game_config.h"
#include "game_engine.h"
#include "sprites.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>

// ============================================
// ESP STARFIGHTER - Main
// ============================================

// --- Melodias ---
const Note melody_intro[] = {
    {523, 100}, {659, 100}, {784, 100},  {1047, 200},
    {0, 50},    {784, 100}, {1047, 300}, {0, 0} // 0 freq = fim
};

// Tema: Imperial March (Star Wars) - Simplificada
const Note melody_game[] = {{392, 500}, {392, 500}, {392, 500}, {311, 350},
                            {466, 150}, {392, 500}, {311, 350}, {466, 150},
                            {392, 500}, {0, 0}};

const Note melody_boss[] = {
    {150, 250}, {200, 250}, {150, 250}, {250, 400}, {0, 0}};

// Melodia triste de Game Over (Marcha Funebre - Chopin)
const Note melody_gameover[] = {{349, 400}, {349, 200}, {349, 200}, {349, 400},
                                {415, 600}, {349, 400}, {349, 200}, {349, 200},
                                {349, 400}, {415, 600}, {392, 400}, {349, 200},
                                {0, 0}};

// Tema: Star Wars - Main Title (Menu)
// Estrutura: {Frequência(Hz), Duração(ms)}
const Note melody_menu[] = {
    {392, 150}, {392, 150}, {392, 150}, // Sol (3 notas rápidas)
    {262, 600},                         // Dó (Grave)
    {392, 600},                         // Sol (Média)
    {349, 150}, {330, 150}, {294, 150}, // Fá, Mi, Ré (Descida rápida)
    {523, 600},                         // Dó (Agudo)
    {392, 300},                         // Sol (Média)
    {349, 150}, {330, 150}, {294, 150}, // Fá, Mi, Ré (Descida rápida)
    {523, 600},                         // Dó (Agudo)
    {392, 300},                         // Sol (Média)
    {349, 150}, {330, 150}, {349, 150}, // Fá, Mi, Fá
    {294, 600},                         // Ré
    {0, 200},                           // Pequena pausa
    {0, 0}                              // Fim da música
};

// --- BMI160 Registers ---
#define BMI160_REG_CHIP_ID 0x00
#define BMI160_REG_ACC_X_LSB 0x12
#define BMI160_REG_CMD 0x7E
#define BMI160_CMD_ACC_NORMAL 0x11

// --- Display OLED ---
TwoWire I2C_OLED = TwoWire(0);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_OLED, OLED_RST);

// --- Acelerômetro BMI160 ---
TwoWire I2C_BMI = TwoWire(1);
bool bmi160Available = false;

// --- Funções BMI160 I2C Manual ---
void bmi160_write(uint8_t reg, uint8_t data) {
  I2C_BMI.beginTransmission(BMI160_ADDR);
  I2C_BMI.write(reg);
  I2C_BMI.write(data);
  I2C_BMI.endTransmission();
}

void bmi160_read(uint8_t reg, uint8_t *data, uint8_t len) {
  I2C_BMI.beginTransmission(BMI160_ADDR);
  I2C_BMI.write(reg);
  I2C_BMI.endTransmission(false);
  I2C_BMI.requestFrom((uint8_t)BMI160_ADDR, len);
  for (uint8_t i = 0; i < len && I2C_BMI.available(); i++) {
    data[i] = I2C_BMI.read();
  }
}

bool bmi160_init() {
  // Soft reset
  bmi160_write(BMI160_REG_CMD, 0xB6);
  delay(100);

  // Check chip ID
  uint8_t chipId = 0;
  bmi160_read(BMI160_REG_CHIP_ID, &chipId, 1);
  if (chipId != 0xD1) {
    Serial.printf("BMI160 Chip ID invalido: 0x%02X\n", chipId);
    return false;
  }

  // Set accelerometer to normal mode
  bmi160_write(BMI160_REG_CMD, BMI160_CMD_ACC_NORMAL);
  delay(50);

  return true;
}

void bmi160_getAccel(int16_t *ax, int16_t *ay, int16_t *az) {
  uint8_t data[6];
  bmi160_read(BMI160_REG_ACC_X_LSB, data, 6);
  *ax = (int16_t)(data[1] << 8 | data[0]);
  *ay = (int16_t)(data[3] << 8 | data[2]);
  *az = (int16_t)(data[5] << 8 | data[4]);
}

// --- Dados do Jogo (compartilhados entre tasks) ---
GameData game;
SemaphoreHandle_t gameMutex;

// --- Protótipos das Tasks ---
void taskInput(void *pvParameters);
void taskGame(void *pvParameters);
void taskRender(void *pvParameters);
void taskAudio(void *pvParameters);

// --- Funções de Renderização ---
void render_intro();
void render_menu();
void render_game();
void render_hud();
void render_gameover();

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP STARFIGHTER v1.0 ===");

  // Mutex para acesso seguro aos dados do jogo
  gameMutex = xSemaphoreCreateMutex();

  // --- Inicializa I2C do OLED ---
  I2C_OLED.begin(OLED_SDA, OLED_SCL, 400000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED falhou!");
    for (;;)
      ;
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.display();
  Serial.println("OLED OK");

  // --- Inicializa I2C do BMI160 ---
  I2C_BMI.begin(BMI160_SDA, BMI160_SCL, 400000);
  bmi160Available = bmi160_init();
  if (bmi160Available) {
    Serial.println("BMI160 OK");
  } else {
    Serial.println("BMI160 nao encontrado - usando botoes para controle");
  }
  // --- Inicializa Botão ---
  pinMode(PIN_BTN_FIRE, INPUT_PULLUP); // Tiro + Especial (segurar)

  // --- Inicializa LEDs e Buzzer ---
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_LED_R, LOW);
  digitalWrite(PIN_LED_G, LOW);
  digitalWrite(PIN_LED_B, LOW);

  // --- Inicializa Estado do Jogo ---
  game_init(&game);

  // --- Cria Tasks FreeRTOS ---
  xTaskCreatePinnedToCore(taskInput, "Input", 2048, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(taskAudio, "Audio", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskGame, "Game", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskRender, "Render", 4096, NULL, 3, NULL, 1);

  Serial.println("Tasks criadas. Iniciando jogo...");
}

void loop() {
  // Loop vazio - tudo gerenciado pelo FreeRTOS
  vTaskDelay(pdMS_TO_TICKS(1000));
}

// ============================================
// TASK: INPUT (Core 0)
// ============================================
void taskInput(void *pvParameters) {
  int16_t ax = 0, ay = 0, az = 0;

  while (1) {
    // Lê acelerômetro se disponível
    if (bmi160Available) {
      bmi160_getAccel(&ax, &ay, &az);
    }

    // Lê botão (invertido por causa do PULLUP)
    bool fire = !digitalRead(PIN_BTN_FIRE);

    // Atualiza dados do jogo (protegido por mutex)
    if (xSemaphoreTake(gameMutex, portMAX_DELAY)) {
      game.accelX = ax;
      game.accelY = ay;
      game.btnFirePrev = game.btnFire;
      game.btnFire = fire;

      // Lógica de Hold para Especial/Laser (V2.5 - Auto Fire)
      if (fire) {
        game.btnHoldTimer += 10; // +10ms a cada loop
        if (game.btnHoldTimer >= 1000 && !game.laserFired) {
          game.laserTriggerRequest = true; // Solicita disparo imediato
          game.laserFired = true;          // Trava
        }
        game.isCharging = (game.btnHoldTimer > 200 && !game.laserFired);
      } else {
        // Se soltou rápido (<1s) e não disparou laser, dispara tiro normal
        if (game.btnFirePrev && !game.laserFired && game.btnHoldTimer < 1000) {
          game.normalFireRequest = true;
        }
        game.btnHoldTimer = 0;
        game.isCharging = false;
        game.laserFired = false; // Destrava ao soltar
      }

      xSemaphoreGive(gameMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(10)); // 100Hz input polling
  }
}

// ============================================
// TASK: GAME LOGIC (Core 1)
// ============================================
void taskGame(void *pvParameters) {
  TickType_t lastWakeTime = xTaskGetTickCount();

  while (1) {
    if (xSemaphoreTake(gameMutex, portMAX_DELAY)) {
      switch (game.state) {
      case STATE_INTRO:
        game.introFrame++;
        if (game.introFrame > 150 || game.btnFire) {
          game.state = STATE_MENU;
          game.introFrame = 0;
          game.currentMelody = 5; // Música do Menu
          game.melodyNoteIndex = 0;
          game.melodyNextTime = millis();
        }
        break;

      case STATE_MENU:
        if (game.btnFire && !game.btnFirePrev) {
          game_reset(&game);
          game.state = STATE_PLAYING;
          game.currentMelody = 0; // Sem música de fase, apenas SFX
          game.melodyNoteIndex = 0;
          game.melodyNextTime = millis();
        }
        break;

      case STATE_PLAYING:
        game_update(&game);
        break;

      case STATE_GAMEOVER:
        if (game.btnFire && !game.btnFirePrev) {
          game.state = STATE_MENU;
          game.currentMelody = 5; // Música do Menu
          game.melodyNoteIndex = 0;
          game.melodyNextTime = millis();
        }
        break;

      default:
        break;
      }
      game.frameCount++;
      xSemaphoreGive(gameMutex);
    }

    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(FRAME_TIME_MS));
  }
}

// ============================================
// TASK: RENDER (Core 1)
// ============================================
void taskRender(void *pvParameters) {
  TickType_t lastWakeTime = xTaskGetTickCount();

  while (1) {
    if (xSemaphoreTake(gameMutex, portMAX_DELAY)) {
      display.clearDisplay();

      switch (game.state) {
      case STATE_INTRO:
        render_intro();
        break;
      case STATE_MENU:
        render_menu();
        break;
      case STATE_PLAYING:
        display.invertDisplay(game.isDay); // Ciclo Dia/Noite
        render_hud();
        render_game();
        break;
      case STATE_GAMEOVER:
        display.invertDisplay(false);
        render_gameover();
        break;
      default:
        break;
      }

      display.display();
      xSemaphoreGive(gameMutex);
    }

    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(FRAME_TIME_MS));
  }
}

// ============================================
// TASK: AUDIO & LED FEEDBACK (Core 0)
// ============================================
void taskAudio(void *pvParameters) {
  while (1) {
    if (xSemaphoreTake(gameMutex, portMAX_DELAY)) {
      // 1. Controle do Buzzer (eventos pontuais)
      if (game.soundDur > 0) {
        tone(PIN_BUZZER, game.soundFreq);
        game.soundDur -= 30;
      } else if (game.soundDur > -100) { // Margem de segurança para desligar
        noTone(PIN_BUZZER);
        game.soundDur = -1000; // Marca como "desligado" para liberar música
      }

      // 2. Sistema de Música de Fundo (melhorado)
      if (game.currentMelody > 0 && game.soundDur <= 0) {
        const Note *mel = (game.currentMelody == 1)   ? melody_intro
                          : (game.currentMelody == 2) ? melody_game
                          : (game.currentMelody == 3) ? melody_boss
                          : (game.currentMelody == 4) ? melody_gameover
                                                      : melody_menu;
        if (millis() >= game.melodyNextTime) {
          Note n = mel[game.melodyNoteIndex];
          if (n.freq == 0 && n.dur == 0) {
            if (game.currentMelody == 1 || game.currentMelody == 4)
              game.currentMelody = 0; // Intro e Game Over tocam 1x
            else
              game.melodyNoteIndex = 0; // Loop (Boss e Menu)
          } else {
            if (n.freq > 0)
              tone(PIN_BUZZER, n.freq, n.dur);
            game.melodyNextTime = millis() + n.dur + 20;
            game.melodyNoteIndex++;
          }
        }
      }

      // 3. Controle do LED RGB
      if (game.isCharging) {
        // Pisca azul progressivamente: 150ms -> 30ms (conforme carrega)
        uint32_t interval = max((uint32_t)30, 150 - (game.btnHoldTimer / 7));
        bool blink = (millis() / interval) % 2 == 0;
        digitalWrite(PIN_LED_B, blink ? HIGH : LOW);
        digitalWrite(PIN_LED_R, LOW);
        digitalWrite(PIN_LED_G, LOW);
      } else if (game.ledTimer > 0) {
        game.ledTimer -= 30;
        digitalWrite(PIN_LED_R,
                     (game.ledColor == 1 || game.ledColor == 4) ? HIGH : LOW);
        digitalWrite(PIN_LED_G,
                     (game.ledColor == 2 || game.ledColor == 4) ? HIGH : LOW);
        digitalWrite(PIN_LED_B,
                     (game.ledColor == 3 || game.ledColor == 4) ? HIGH : LOW);
      } else {
        // TODOS OS LEDs OFF por padrão
        digitalWrite(PIN_LED_R, LOW);
        digitalWrite(PIN_LED_G, LOW);
        digitalWrite(PIN_LED_B, LOW);

        // Pisca azul no menu
        if (game.state == STATE_MENU || game.state == STATE_INTRO) {
          digitalWrite(PIN_LED_B, (millis() / 500) % 2);
        }
      }
      xSemaphoreGive(gameMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(30));
  }
}

// ============================================
// RENDERIZAÇÃO
// ============================================

void render_intro() {
  int frame = game.introFrame;

  // Animação ASCII da nave "decolando"
  if (frame < 20) {
    // Fase 1: Nave aparece de baixo
    int shipY = 64 - frame * 2;
    display.drawBitmap(56, shipY, sprite_player, 16, 12, WHITE);
  } else if (frame < 50) {
    // Fase 2: Título digitando letra por letra
    display.drawBitmap(56, 24, sprite_player, 16, 12, WHITE);

    const char *title = "ESP WARS";
    int lettersToShow = min((frame - 20) / 5, 8); // 8 letras para "ESP WARS"
    display.setTextSize(2);
    display.setCursor(15, 0);
    for (int i = 0; i < lettersToShow; i++) {
      display.print(title[i]);
    }
  } else if (frame < 90) {
    // Fase 3: Subtítulo aparecendo
    display.setTextSize(2);
    display.setCursor(15, 0);
    display.print("ESP WARS");
    display.drawBitmap(56, 24, sprite_player, 16, 12, WHITE);

    const char *subtitle = "O PROTETOR ESTELAR";
    int lettersToShow = min((frame - 50) / 2, 16);
    display.setTextSize(1);
    display.setCursor(8, 40);
    for (int i = 0; i < lettersToShow; i++) {
      display.print(subtitle[i]);
    }
  } else {
    // Fase 4: Tela completa, pisca "APERTE TIRO"
    display.setTextSize(2);
    display.setCursor(15, 0);
    display.print("ESP WARS");
    display.setTextSize(1);
    display.setCursor(8, 40);
    display.print("O PROTETOR ESTELAR");
    display.drawBitmap(56, 24, sprite_player, 16, 12, WHITE);

    if ((frame / 15) % 2 == 0) {
      display.setCursor(22, 56);
      display.print("APERTE O TIRO");
    }
  }

  // Inicia música de intro
  if (frame == 1) {
    game.currentMelody = 1;
    game.melodyNoteIndex = 0;
    game.melodyNextTime = millis();
  }
}

void render_menu() {
  display.setTextSize(1);
  display.setCursor(15, 5);
  display.print("ESQUADRAO ESP32");

  display.drawLine(0, 14, 128, 14, WHITE);

  display.setCursor(30, 25);
  display.print("> INICIAR <");

  display.setCursor(15, 45);
  display.print("Incline p/ mover");
  display.setCursor(15, 55);
  display.print("TIRO p/ atacar");

  // Nave decorativa animada
  int offset = (game.frameCount / 10) % 3;
  display.drawBitmap(5, 22 + offset, sprite_player, 16, 12, WHITE);
  display.drawBitmap(107, 22 + offset, sprite_player, 16, 12, WHITE);
}

void render_hud() {
  // Área amarela (0-15)
  // Score
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("*");
  display.setCursor(8, 0);
  display.printf("%06d", game.player.score);

  // Vidas
  for (int i = 0; i < game.player.lives; i++) {
    display.drawBitmap(60 + i * 10, 0, icon_heart, 8, 7, WHITE);
  }

  // Level
  display.setCursor(100, 0);
  display.printf("N%d", game.player.level);

  // Shield bar
  display.setCursor(0, 9);
  display.print("EG");
  int shieldWidth = map(game.player.shield, 0, PLAYER_MAX_SHIELD, 0, 40);
  display.drawRect(15, 9, 42, 6, WHITE);
  display.fillRect(16, 10, shieldWidth, 4, WHITE);

  // Linha divisória
  display.drawLine(0, 15, 128, 15, WHITE);
}

void render_game() {
  // Área azul (16-63)

  // Screen Shake effect
  int shakeX = (game.shakeTimer > 0) ? random(-2, 3) : 0;
  int shakeY = (game.shakeTimer > 0) ? random(-1, 2) : 0;
  if (game.shakeTimer > 0)
    game.shakeTimer--;

  // Estrelas de fundo (parallax horizontal)
  for (int i = 0; i < 15; i++) {
    // Efeito de velocidade diferente para as estrelas (parallax)
    int speed = (i % 3) + 1;
    int x = (i * 37 - (game.frameCount * speed)) % 128;
    if (x < 0)
      x += 128; // Mantém na tela
    int y = GAME_AREA_Y + ((i * 23) % GAME_AREA_HEIGHT);
    display.drawPixel(x + shakeX, y + shakeY, WHITE);
  }

  // Desenha nave do jogador
  if (game.player.isAlive) {
    const unsigned char *shipSprite =
        (game.frameCount % 10 < 5) ? sprite_player_thrust : sprite_player;
    display.drawBitmap((int)game.player.x + shakeX, (int)game.player.y + shakeY,
                       shipSprite, PLAYER_WIDTH, PLAYER_HEIGHT, WHITE);

    // Desenha efeito de carga
    if (game.isCharging) {
      int r = (game.frameCount % 5) + 8;
      display.drawCircle((int)game.player.x + 8, (int)game.player.y + 6, r,
                         WHITE);
    }

    // Desenha Laser Beam
    if (game.laserTimer > 0) {
      display.fillRect((int)game.player.x + 16, (int)game.player.y + 4, 128, 4,
                       WHITE);
    }
  }

  // Desenha tiros
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (game.bullets[i].active) {
      display.drawBitmap((int)game.bullets[i].x, (int)game.bullets[i].y,
                         sprite_bullet, 4, 2, WHITE);
    }
  }

  // Desenha inimigos
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (game.enemies[i].active) {
      if (game.enemies[i].type == 10) { // BOSS
        display.drawBitmap((int)game.enemies[i].x, (int)game.enemies[i].y,
                           sprite_boss, 32, 32, WHITE);
        // Barra de vida do Boss
        display.drawRect((int)game.enemies[i].x, (int)game.enemies[i].y - 4, 32,
                         3, WHITE);
        int hpWidth = map(game.enemies[i].health, 0, 100, 0, 32);
        display.fillRect((int)game.enemies[i].x, (int)game.enemies[i].y - 4,
                         hpWidth, 3, WHITE);
      } else {
        const unsigned char *enemySprite =
            (game.enemies[i].type == 0) ? sprite_enemy1 : sprite_enemy2;
        display.drawBitmap((int)game.enemies[i].x, (int)game.enemies[i].y,
                           enemySprite, 8, 8, WHITE);
      }
    }
  }

  // Desenha Power-Ups
  for (int i = 0; i < 3; i++) {
    if (game.powerups[i].active) {
      // Icone simples: quadrado piscando
      if ((game.frameCount / 5) % 2 == 0) {
        display.fillRect((int)game.powerups[i].x + shakeX,
                         (int)game.powerups[i].y + shakeY, 6, 6, WHITE);
      } else {
        display.drawRect((int)game.powerups[i].x + shakeX,
                         (int)game.powerups[i].y + shakeY, 6, 6, WHITE);
      }
    }
  }

  // Desenha tiros inimigos (Plasma Vermelho/Círculos)
  for (int i = 0; i < 20; i++) {
    if (game.enemyBullets[i].active) {
      display.drawCircle((int)game.enemyBullets[i].x,
                         (int)game.enemyBullets[i].y, 2, WHITE);
    }
  }

  // Efeito de Flash do Laser
  if (game.flashTimer > 0) {
    display.fillScreen(WHITE);
  }
}

void render_gameover() {
  display.setTextSize(2);
  display.setCursor(10, 0);
  display.print("GAME OVER");

  display.setTextSize(1);
  display.setCursor(25, 30);
  display.printf("PONTOS: %d", game.player.score);

  display.setCursor(25, 45);
  display.printf("NIVEL: %d", game.player.level);

  if ((game.frameCount / 20) % 2 == 0) {
    display.setCursor(20, 56);
    display.print("APERTE O TIRO");
  }
}

// ============================================
// GAME ENGINE IMPLEMENTATION
// ============================================

void game_init(GameData *g) {
  g->state = STATE_INTRO;
  g->frameCount = 0;
  g->introFrame = 0;
  g->introComplete = false;
  g->accelX = 0;
  g->accelY = 0;
  g->btnFire = false;
  g->btnFirePrev = false;
  g->btnHoldTimer = 0;
  g->flashTimer = 0;
  g->level = 1;

  g->ledTimer = 0;
  g->ledColor = 0;
  g->soundFreq = 0;
  g->soundDur = 0;
  g->specialCooldown = 0;
  g->isDay = false;
  g->lastCycleChange = 0;
  g->bossActive = false;

  g->laserTimer = 0;
  g->isCharging = false;
  g->laserTriggerRequest = false;
  g->laserFired = false;

  g->level = 1; // Nivel inicial
  g->normalFireRequest = false;
  g->btnFireReleased = false;

  // Novos campos v2.0
  g->currentMelody = 0;
  g->melodyNoteIndex = 0;
  g->melodyNextTime = 0;
  g->shakeTimer = 0;
  g->highScore = 0; // TODO: Ler da EEPROM
  g->hasTiroDuplo = false;
  g->hasTurbo = false;
  g->buffTiroDuploTimer = 0;
  g->buffTurboTimer = 0;
  for (int i = 0; i < 3; i++) {
    g->powerups[i].active = false;
  }

  player_init(&g->player);

  for (int i = 0; i < MAX_BULLETS; i++) {
    g->bullets[i].active = false;
  }
  for (int i = 0; i < MAX_ENEMIES; i++) {
    g->enemies[i].active = false;
  }
}

void game_reset(GameData *g) {
  player_init(&g->player);

  for (int i = 0; i < MAX_BULLETS; i++) {
    g->bullets[i].active = false;
  }
  for (int i = 0; i < MAX_ENEMIES; i++) {
    g->enemies[i].active = false;
  }
  for (int i = 0; i < 20; i++)
    g->enemyBullets[i].active = false;

  g->bossActive = false;
  g->level = 1;
  g->frameCount = 0;
  g->laserTriggerRequest = false;
  g->isCharging = false;
  g->laserFired = false;
  g->flashTimer = 0;
}

void game_update(GameData *g) {
  // Atualiza jogador com entrada do acelerômetro
  player_update(&g->player, g->accelX, g->accelY);

  // Disparo (Refatorado para evitar pontos duplicados)
  if (g->normalFireRequest) {
    player_fire(g);
    g->normalFireRequest = false;
  }

  // Atualiza tiros
  bullet_update(g->bullets, MAX_BULLETS);
  enemy_bullet_update(g->enemyBullets, 20); // Atualiza tiros inimigos

  // Atualiza inimigos (Passando ponteiro do jogo para acessar balas)
  // Precisei mudar a assinatura ou fazer a logica aqui?
  // Vou manter a chamada e alterar a funcao enemy_update para receber GameData
  // ou fazer a logica do boss aqui fora? Melhor: fazer a logica do Boss AQUI
  // para ter acesso a tudo, ou passar GameData para enemy_update. Vou passar o
  // g para enemy_update. Mas enemy_update está definida como (Enemy[], int).
  // Vou alterar o protótipo depois na próxima chamada. Por enquanto, vou mover
  // a lógica do Boss para cá ou alterar a função. Vou alterar a função
  // enemy_update para receber GameData* g. Mas espera, isso requer mudar o
  // header. Vou fazer a lógica de ataque do Boss AQUI em game_update para
  // simplificar sem mudar muitos arquivos.

  // Apenas movimento basico em enemy_update, ataque aqui.
  enemy_update(g->enemies, MAX_ENEMIES);

  // Boss AI (Ataque)
  if (g->bossActive) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (g->enemies[i].active && g->enemies[i].type == 10) {
        // Movimento mais complexo (sobrescrevendo o basico)
        g->enemies[i].y = (SCREEN_HEIGHT / 2) + sin(g->frameCount * 0.05) * 20;

        // Ataques
        if (g->frameCount % 60 == 0) { // Ataque a cada 1s (aprox)
          // Tiro Triplo
          enemy_bullet_spawn(g->enemyBullets, g->enemies[i].x, g->enemies[i].y,
                             -2.5, 0);
          enemy_bullet_spawn(g->enemyBullets, g->enemies[i].x, g->enemies[i].y,
                             -2.0, -1.0);
          enemy_bullet_spawn(g->enemyBullets, g->enemies[i].x, g->enemies[i].y,
                             -2.0, 1.0);
          g->soundFreq = 100;
          g->soundDur = 50; // Boomzinho
        }
      }
    }
  }

  // Spawn de inimigos (Normal ou Boss)
  if (!g->bossActive && g->player.score >= 5000 &&
      (g->player.score % 5000 < 500)) {
    g->bossActive = true;
    enemy_spawn(g, 128, 16, 10); // Tipo 10 = BOSS
    g->currentMelody = 3;        // Música de Boss!
    g->melodyNoteIndex = 0;
    g->soundFreq = 800;
    g->soundDur = 1000; // Som de alerta
  }

  if (game.frameCount % max(20, (60 - (int)g->level * 5)) == 0 &&
      !g->bossActive) {
    float spawnY = random(GAME_AREA_Y, SCREEN_HEIGHT - 8);
    uint8_t type = random(0, 2);
    enemy_spawn(g, 128, spawnY, type);
  }

  if (g->flashTimer > 0)
    g->flashTimer--;

  // Ciclo Dia/Noite (Cada 30 seg aprox @ 30fps = 900 frames)
  if (g->frameCount - g->lastCycleChange > 900) {
    g->isDay = !g->isDay;
    g->lastCycleChange = g->frameCount;
    g->ledColor = 4;
    g->ledTimer = 500; // Pisca branco na troca
  }

  // Laser Beam Logic
  // Checa solicitação de disparo vinda do taskInput
  if (g->laserTriggerRequest) {
    // Dispara Laser
    g->laserTimer = 10; // Laser dura 10 frames
    g->soundFreq = 100;
    g->soundDur = 500; // Som grave e longo
    g->ledColor = 4;   // Branco como pedido
    g->ledTimer = 100;
    g->flashTimer = 2; // 2 frames de flash branco intenso
    g->shakeTimer = 10;

    // Hitscan: Destroi tudo na linha do player
    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (g->enemies[i].active && abs(g->enemies[i].y - g->player.y) < 20) {
        g->enemies[i].active = false;
        g->player.score += 50;
        // TODO: Explosão visual
      }
    }
    g->laserTriggerRequest = false; // Consome o request
  }

  if (g->laserTimer > 0)
    g->laserTimer--;

  // Colisões tiro-inimigo
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!g->bullets[i].active)
      continue;

    for (int j = 0; j < MAX_ENEMIES; j++) {
      if (!g->enemies[j].active)
        continue;

      int targetW = (g->enemies[j].type == 10) ? 32 : 8;
      int targetH = (g->enemies[j].type == 10) ? 32 : 8;

      if (check_collision(g->bullets[i].x, g->bullets[i].y, 4, 2,
                          g->enemies[j].x, g->enemies[j].y, targetW, targetH)) {
        g->bullets[i].active = false;
        g->enemies[j].health--;

        // Efeito de acerto (feedback azul)
        g->ledColor = 3;
        g->ledTimer = 100;
        g->soundFreq = 2000;
        g->soundDur = 50;

        if (g->enemies[j].health <= 0) {
          g->enemies[j].active = false;
          if (g->enemies[j].type == 10) {
            g->player.score += 2000;
            g->bossActive = false;
            g->player.score += 2000;
            g->bossActive = false;
            g->currentMelody = 6; // Vitória!
            g->melodyNoteIndex = 0;
            g->soundFreq = 1500;
            g->level++;        // Sobe de nível!
            g->soundDur = 500; // Vitória Boss
          } else {
            g->player.score += 100;

            // Spawn de Power-Up (Aumentado para 10% para teste/correção)
            if (random(0, 100) < 10) {
              for (int k = 0; k < 3; k++) {
                if (!g->powerups[k].active) {
                  g->powerups[k].x = g->enemies[j].x;
                  g->powerups[k].y = g->enemies[j].y;
                  g->powerups[k].type = random(0, 4);
                  g->powerups[k].active = true;
                  break;
                }
              }
            }
          }
        }
        break;
      }
    }
  }

  // Atualiza Power-Ups
  for (int i = 0; i < 3; i++) {
    if (!g->powerups[i].active)
      continue;
    g->powerups[i].x -= 0.5; // Move para esquerda
    if (g->powerups[i].x < -8)
      g->powerups[i].active = false;

    // Colisão com jogador
    if (check_collision(g->player.x, g->player.y, PLAYER_WIDTH, PLAYER_HEIGHT,
                        g->powerups[i].x, g->powerups[i].y, 8, 8)) {
      g->powerups[i].active = false;
      g->ledColor = 2;
      g->ledTimer = 300; // Flash verde
      g->soundFreq = 1000;
      g->soundDur = 100;

      switch (g->powerups[i].type) {
      case 0:
        g->player.shield = min(g->player.shield + 50, PLAYER_MAX_SHIELD);
        break;
      case 1:
        g->hasTiroDuplo = true;
        g->buffTiroDuploTimer = 300;
        break;
      case 2:
        g->hasTurbo = true;
        g->buffTurboTimer = 240;
        break;
      case 3:
        g->specialCooldown = 0;
        break;
      }
    }
  }

  // Decrementa timers de buff
  if (g->buffTiroDuploTimer > 0)
    g->buffTiroDuploTimer--;
  else
    g->hasTiroDuplo = false;
  if (g->buffTurboTimer > 0)
    g->buffTurboTimer--;
  else
    g->hasTurbo = false;

  // Colisões Tiro Inimigo vs Player
  for (int i = 0; i < 20; i++) {
    if (g->enemyBullets[i].active) {
      if (check_collision(g->enemyBullets[i].x, g->enemyBullets[i].y, 3, 3,
                          g->player.x, g->player.y, PLAYER_WIDTH,
                          PLAYER_HEIGHT)) {
        g->enemyBullets[i].active = false;
        player_takeDamage(&g->player, 25); // Dano do tiro inimigo
      }
    }
  }

  // Colisão Player-Inimigo (Kamikaze)
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!g->enemies[i].active)
      continue;

    if (check_collision(g->player.x, g->player.y, PLAYER_WIDTH, PLAYER_HEIGHT,
                        g->enemies[i].x, g->enemies[i].y, 8, 8)) {
      g->enemies[i].active = false;
      player_takeDamage(&g->player, 30);

      if (g->player.lives <= 0) {
        g->state = STATE_GAMEOVER;
        g->currentMelody = 4; // Game Over Music
        g->melodyNoteIndex = 0;
        g->melodyNextTime = millis();
      }
    }
  }

  // Level up a cada 1000 pontos
  uint8_t newLevel = 1 + (g->player.score / 1000);
  if (newLevel > g->player.level) {
    g->player.level = newLevel;
  }
}

void player_init(Player *p) {
  p->x = PLAYER_START_X;
  p->y = PLAYER_START_Y;
  p->lives = PLAYER_MAX_LIVES;
  p->shield = PLAYER_MAX_SHIELD;
  p->score = 0;
  p->isAlive = true;
  p->thrustOn = true;
  p->level = 1;
}

void player_update(Player *p, float accelX, float accelY) {
  if (!p->isAlive)
    return;

  // Aplica zona morta
  float moveX = 0;
  float moveY = 0;

  // No modo SIDE-SCROLLER:
  // accelY inclina a nave VERTICALMENTE (para cima/baixo)
  // accelX inclina a nave HORIZONTALMENTE (frente/trás)
  if (abs(accelY) > ACCEL_DEADZONE) {
    moveY = (accelY / ACCEL_SENSITIVITY) * PLAYER_SPEED;
  }
  if (abs(accelX) > ACCEL_DEADZONE) {
    moveX = (accelX / ACCEL_SENSITIVITY) * PLAYER_SPEED;
  }

  // Atualiza posição
  p->x += moveX;
  p->y += moveY;

  // Limites da tela
  if (p->x < 0)
    p->x = 0;
  if (p->x > SCREEN_WIDTH / 2) // Limita o player à metade esquerda da tela
    p->x = SCREEN_WIDTH / 2;
  if (p->y < GAME_AREA_Y)
    p->y = GAME_AREA_Y;
  if (p->y > SCREEN_HEIGHT - PLAYER_HEIGHT)
    p->y = SCREEN_HEIGHT - PLAYER_HEIGHT;
}

void player_fire(GameData *g) {
  bullet_spawn(g->bullets, g->player.x + PLAYER_WIDTH,
               g->player.y + PLAYER_HEIGHT / 2 - 1);

  // Tiro Duplo se buff ativo
  if (g->hasTiroDuplo) {
    bullet_spawn(g->bullets, g->player.x + PLAYER_WIDTH,
                 g->player.y + PLAYER_HEIGHT / 2 + 4);
  }

  g->ledColor = 3;
  g->ledTimer = 100;   // Flash azul visível
  g->soundFreq = 4000; // Frequência original (melhor ouvida)
  g->soundDur = 50;    // 50ms (curto mas audível)
}

void player_takeDamage(Player *p, int damage) {
  p->shield -= damage;
  // Feedback de dano
  game.ledColor = 1;
  game.ledTimer = 300; // Flash vermelho
  game.soundFreq = 150;
  game.soundDur = 150; // Som grave mais curto
  game.shakeTimer = 6; // Screen shake!

  if (p->shield <= 0) {
    p->shield = PLAYER_MAX_SHIELD;
    p->lives--;
    if (p->lives <= 0) {
      p->isAlive = false;
    }
  }
}

void bullet_update(Bullet bullets[], int count) {
  for (int i = 0; i < count; i++) {
    if (!bullets[i].active)
      continue;

    bullets[i].x += bullets[i].speedX;

    if (bullets[i].x > SCREEN_WIDTH) {
      bullets[i].active = false;
    }
  }
}

void bullet_spawn(Bullet bullets[], float x, float y) {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      bullets[i].x = x;
      bullets[i].y = y;
      bullets[i].speedX = BULLET_SPEED; // Atira para a direita
      bullets[i].speedY = 0;
      bullets[i].active = true;
      break;
    }
  }
}

void enemy_bullet_update(EnemyProjectile bullets[], int count) {
  for (int i = 0; i < count; i++) {
    if (!bullets[i].active)
      continue;
    bullets[i].x += bullets[i].speedX;
    bullets[i].y += bullets[i].speedY;
    if (bullets[i].x < 0 || bullets[i].y < 0 || bullets[i].y > SCREEN_HEIGHT) {
      bullets[i].active = false;
    }
  }
}

void enemy_bullet_spawn(EnemyProjectile bullets[], float x, float y, float vx,
                        float vy) {
  for (int i = 0; i < 20; i++) {
    if (!bullets[i].active) {
      bullets[i].x = x;
      bullets[i].y = y;
      bullets[i].speedX = vx;
      bullets[i].speedY = vy;
      bullets[i].active = true;
      break;
    }
  }
}

void enemy_update(Enemy enemies[], int count) {
  for (int i = 0; i < count; i++) {
    if (!enemies[i].active)
      continue;

    if (enemies[i].type == 10) { // IA do BOSS
      // Move-se verticalmente na borda direita
      enemies[i].y += enemies[i].speedY;
      if (enemies[i].y <= GAME_AREA_Y || enemies[i].y >= SCREEN_HEIGHT - 32) {
        enemies[i].speedY = -enemies[i].speedY;
      }
      // Ataca raramente? (Implementar no futuro)
    } else {
      enemies[i].x -= enemies[i].speedX; // Move para a esquerda
      enemies[i].y += enemies[i].speedY;

      // Inverte direção vertical nas bordas da área de jogo
      if (enemies[i].y <= GAME_AREA_Y || enemies[i].y >= SCREEN_HEIGHT - 8) {
        enemies[i].speedY = -enemies[i].speedY;
      }
    }

    // Remove se sair da tela pela esquerda
    if (enemies[i].x < -32) {
      enemies[i].active = false;
    }
  }
}

void enemy_spawn(GameData *g, float x, float y, uint8_t type) {
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!g->enemies[i].active) {
      g->enemies[i].x = x;
      g->enemies[i].y = y;
      g->enemies[i].type = type;
      if (type == 10) { // Configuração BOSS
        g->enemies[i].speedX = 0;
        g->enemies[i].speedY = 1.0;
        g->enemies[i].health = 100;
        g->enemies[i].x = 90; // Posiciona na direita mas visível
      } else {
        g->enemies[i].speedX =
            1.5 + (g->level * 0.2); // Velocidade aumenta com nível
        g->enemies[i].speedY = (random(0, 2) == 0) ? 0.5 : -0.5;
        if (type == 1)
          g->enemies[i].speedY *= 2;
        g->enemies[i].health = g->level; // Vida baseada no nível
      }
      g->enemies[i].active = true;
      break;
    }
  }
}

bool check_collision(float x1, float y1, int w1, int h1, float x2, float y2,
                     int w2, int h2) {
  return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}
