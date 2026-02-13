#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// Configuração para ESP32-WROOM (HW-724)
// SDA: GPIO 5
// SCL: GPIO 4
// RST: GPIO 16 (conforme DOCUMENTACAO_PLACA.md)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/16, /* clock=*/4,
                                         /* data=*/5);

// Inclui o arquivo com os bitmaps baixados do repositório original
#include "bitmaps.h"

void setup(void) {
  // Inicializa o barramento I2C com os pinos corretos
  Wire.begin(5, 4);
  u8g2.begin();
}

void loop(void) {
  // Chama a função loop_mochi que foi renomeada no bitmaps.h
  // ou implementa a lógica aqui se preferir.

  // Como transformamos o .ino original em bitmaps.h e renomeamos as funções:
  loop_mochi();
}
