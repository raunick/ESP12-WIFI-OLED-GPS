#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

// ============================================
// ESP STARFIGHTER - Configurações do Jogo
// ============================================

// --- Display OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define HUD_HEIGHT 16       // Área amarela (status)
#define GAME_AREA_Y 16      // Início da área azul
#define GAME_AREA_HEIGHT 48 // Altura da área de jogo

// --- Pinos OLED (I2C Bus 1) ---
#define OLED_SDA 5
#define OLED_SCL 4
#define OLED_RST 16
#define OLED_ADDR 0x3C

// --- Pinos BMI160 (I2C Bus 2) ---
#define BMI160_SDA 25 // Remapeado: Único disponível
#define BMI160_SCL 26 // Remapeado: Único disponível
#define BMI160_ADDR 0x69

// --- Pinos de Controle ---
#define PIN_BTN_FIRE 15 // Tiro (tap) + Especial (segurar 1s)
// PIN_BTN_BOMB removido - usando hold no botão de tiro

// --- Pinos de Áudio/Feedback ---
#define PIN_BUZZER 2 // Buzzer (compartilha com LED onboard)
#define PIN_LED_R 13
#define PIN_LED_G 12
#define PIN_LED_B 14

// --- Matriz LED 8x8 (SPI) ---
// Usando pinos de propósito geral disponíveis
// #define PIN_MATRIX_DIN    16      // GPIO 16
// #define PIN_MATRIX_CLK    15      // GPIO 15 (Conflito? Vou usar 16 e
// 17/RX/TX? melhor não mexer agora) #define PIN_MATRIX_CS     5       //
// Conflito com OLED.

// ATENÇÃO: Vou redefinir para pins livres
#define PIN_MATRIX_DIN 25 // Só se não estiver usando o BMI160
#define PIN_MATRIX_CLK 26
#define PIN_MATRIX_CS 27 // Se existir? Não, imagem mostra SVP, SVN...

// --- Configurações do Jogo ---
#define TARGET_FPS 30
#define FRAME_TIME_MS (1000 / TARGET_FPS)

// --- Configurações do Player ---
#define PLAYER_WIDTH 16
#define PLAYER_HEIGHT 12
#define PLAYER_START_X 10
#define PLAYER_START_Y 32
#define PLAYER_SPEED 2.5
#define PLAYER_MAX_LIVES 3
#define PLAYER_MAX_SHIELD 100

// --- Configurações de Inimigos ---
#define MAX_ENEMIES 8
#define ENEMY_SPEED 2.0
#define ENEMY_SPAWN_RATE 80 // frames entre spawns

// --- Configurações de Tiros ---
#define MAX_BULLETS 10
#define BULLET_SPEED 4.0

// --- Estados do Jogo ---
enum GameState {
  STATE_INTRO,
  STATE_MENU,
  STATE_PLAYING,
  STATE_PAUSED,
  STATE_GAMEOVER
};

#define ACCEL_DEADZONE 2000    // Zona morta para evitar drift
#define ACCEL_SENSITIVITY 8000 // Divisor para conversão em pixels

#endif // GAME_CONFIG_H
