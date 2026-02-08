#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "game_config.h"
#include <Arduino.h>

// ============================================
// ESP STARFIGHTER - Estruturas do Jogo
// ============================================

// --- Estrutura do Jogador ---
struct Player {
  float x;
  float y;
  int lives;
  int shield;
  uint32_t score;
  bool isAlive;
  bool thrustOn; // Animação de propulsão
  uint8_t level;
};

// --- Estrutura de Projétil ---
struct Bullet {
  float x;
  float y;
  bool active;
  float speedX; // Horizontal para side-scroller
  float speedY;
};

// --- Estrutura de Inimigo ---
struct Enemy {
  float x;
  float y;
  bool active;
  uint8_t type; // 0, 1, 2 (Inimigo), 10 (Boss), 20 (Projetil Boss)
  float speedY;
  float speedX;
  int16_t health;
};

// --- Estrutura de Explosão ---
struct Explosion {
  float x;
  float y;
  bool active;
  uint8_t frame;
  uint8_t timer;
};

// --- Estrutura de Nota Musical ---
struct Note {
  uint16_t freq;
  uint16_t dur;
};

// --- Estrutura de Power-Up ---
struct PowerUp {
  float x;
  float y;
  uint8_t type; // 0:Escudo, 1:TiroDuplo, 2:Turbo, 3:BombaExtra
  bool active;
};

// --- Estrutura de Tiro Inimigo (Novo) ---
struct EnemyProjectile {
  float x;
  float y;
  float speedX;
  float speedY;
  bool active;
};

// --- Estado Global do Jogo ---
struct GameData {
  GameState state;
  Player player;
  Bullet bullets[MAX_BULLETS];
  EnemyProjectile enemyBullets[20]; // Máximo 20 tiros na tela
  Enemy enemies[MAX_ENEMIES];
  Explosion explosions[4];

  uint32_t frameCount;
  uint32_t lastFrameTime;
  uint8_t introFrame;
  bool introComplete;

  // Feedback e Efeitos
  // Feedback e Efeitos
  int32_t
      ledTimer; // Tempo restante do efeito LED (int32 para evitar underflow)
  uint8_t ledColor;   // 0:Off, 1:Red, 2:Green, 3:Blue, 4:White
  uint16_t soundFreq; // Frequência do som atual
  int32_t soundDur;   // Duração do som (int32 para evitar underflow)

  // Mecânicas
  uint32_t specialCooldown; // Cooldown da bomba
  bool isDay;               // Modo Dia/Noite
  uint32_t lastCycleChange; // Última troca de ciclo

  // Boss
  bool bossActive;
  uint8_t bossPattern;

  // Power-Ups e Buffs
  PowerUp powerups[3];
  uint32_t buffTiroDuploTimer;
  uint32_t buffTurboTimer;
  bool hasTiroDuplo;
  bool hasTurbo;

  // Música
  // Música
  uint8_t currentMelody; // 0:None, 1:Intro, 2:Game, 3:Boss, 4:GameOver, 5:Menu,
                         // 6:Victory
  uint8_t melodyNoteIndex;
  uint32_t melodyNextTime;

  // Polish
  uint8_t shakeTimer;
  uint32_t highScore;
  uint8_t level;          // Nível atual de dificuldade
  uint8_t flashTimer;     // Timer para animação de flash
  bool normalFireRequest; // Solicitação de tiro normal (blaster)

  // Inputs
  float accelX;
  float accelY;
  bool btnFire;
  bool btnFirePrev;
  uint32_t btnHoldTimer; // Tempo segurando o botão (ms)

  // Laser Beam
  bool laserTriggerRequest; // Flag para solicitar disparo do laser
  uint16_t laserTimer;      // Tempo que o laser fica na tela
  bool isCharging;          // Se está carregando o laser
  bool laserFired;          // Trava para indicar que laser disparou neste hold
  bool btnFireReleased;     // Flag para detectar soltura do botão
};

// --- Protótipos ---
void game_init(GameData *game);
void game_update(GameData *game);
void game_reset(GameData *game);

void player_init(Player *player);
void player_update(Player *player, float accelX, float accelY);
void player_fire(GameData *game);
void player_takeDamage(Player *player, int damage);

void bullet_update(Bullet bullets[], int count);
void bullet_spawn(Bullet bullets[], float x, float y);

void enemy_bullet_update(EnemyProjectile bullets[], int count);
void enemy_bullet_spawn(EnemyProjectile bullets[], float x, float y, float vx,
                        float vy);

void enemy_update(Enemy enemies[], int count);
void enemy_spawn(GameData *g, float x, float y, uint8_t type);

bool check_collision(float x1, float y1, int w1, int h1, float x2, float y2,
                     int w2, int h2);

#endif // GAME_ENGINE_H
