# Documentação da Placa ESP32-WROOM (HW-724) com OLED

Este documento detalha as especificações e conexões da placa ESP32 HW-724.

## 📌 Especificações Técnicas
| Característica | Valor |
|:---|:---|
| **Módulo** | ESP32-WROOM-32 |
| **Processador** | Xtensa® Dual-Core 32-bit LX6 |
| **Clock** | 80MHz a 240MHz |
| **Flash** | 4MB |
| **RAM** | 520KB |
| **Conectividade** | WiFi 802.11 b/g/n + Bluetooth 4.2 |

## 🖥️ Display OLED Integrado (SSD1306)
- **Resolução:** 128x64 pixels
- **Cores:** Bicolor (Amarelo topo 16px + Azul 48px)
- **Endereço I2C:** 0x3C
- **LED RGB Interativo**:
    - **Azul**: Piscando ao atirar.
    - **Vermelho**: Ao sofrer dano ou colidir.
    - **Branco**: Mudança de ciclo Dia/Noite.
    - **Verde**: Status normal de jogo.
- **Buzzer**: Sons agudos para tiros e graves para danos. Alerta de sirene quando o Boss aparece.
- **Ciclo Dia/Noite**: A tela inverte cores a cada 30 segundos para aumentar o desafio.
- **BOSS**: Após 5000 pontos, um inimigo gigante aparece com barra de vida.

| Função | Pino | Etiqueta na Placa |
|:---|:---|:---|
| SDA | GPIO 5 | **5** |
| SCL | GPIO 4 | **4** |
| RST | GPIO 16 | **16** |

---

# 🎮 ESP PROTETOR ESTELAR - Manual do Jogo

## Visão Geral
Jogo espacial estilo Star Fox para o display OLED bicolor. Controle sua nave com o acelerômetro, destrua inimigos e sobreviva o máximo possível!

## Controles (Mapeamento Ajustado)
| Ação | Entrada | Pino | Etiqueta |
|:---|:---|:---|:---|
| Mover nave | Inclinar placa | 25/26 | **25/26** (I2C) |
| Tiro | Buzzer | 0 | Bipe de tiro e dano |
| Botão Especial (Bomba) | 2 | Limpa a tela de inimigos |
| Botão de Tiro | 15 | Disparo laser |

## Pinagem Completa do Projeto

### BMI160 Acelerômetro
| Função | Pino | Etiqueta |
|:---|:---|:---|
| SDA | GPIO 25 | **25** |
| SCL | GPIO 26 | **26** |

### Áudio e Feedback
| Função | Pino | Etiqueta |
|:---|:---|:---|
| Buzzer PWM | GPIO 0 | **0** |
| LED RGB R | GPIO 13 | **13** |
| LED RGB G | GPIO 12 | **12** |
| LED RGB B | GPIO 14 | **14** |

## Arquitetura FreeRTOS
```
CORE 0              CORE 1
┌──────────┐       ┌──────────┐
│taskInput │       │taskRender│
│taskAudio │       │taskGame  │
└──────────┘       └──────────┘
```

## Estados do Jogo (Tradução)
```
INTRO (Introdução) → MENU (Menu Principal) → PLAYING (Jogando) → GAMEOVER (Fim de Jogo)
```
