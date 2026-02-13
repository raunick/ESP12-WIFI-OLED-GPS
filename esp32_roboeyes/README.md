<p align="center">
  <img src="docs/roboeyes_logo.svg" width="120" alt="RoboEyes Logo">
</p>

<h1 align="center">🤖 RoboEyes v5.0</h1>

<p align="center">
  <b>Olhos animados interativos com sensor touch, expressões automáticas e dashboard web</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/platform-ESP32--WROOM-blue?style=flat-square" alt="Platform">
  <img src="https://img.shields.io/badge/framework-Arduino-teal?style=flat-square" alt="Framework">
  <img src="https://img.shields.io/badge/display-OLED%20128x64-yellow?style=flat-square" alt="Display">
  <img src="https://img.shields.io/badge/version-5.0-green?style=flat-square" alt="Version">
  <img src="https://img.shields.io/badge/license-MIT-orange?style=flat-square" alt="License">
</p>

---

## 📖 Sobre

RoboEyes transforma uma placa ESP32-WROOM com display OLED integrado em um robô expressivo. Os olhos animados reagem à temperatura, à inclinação e ao toque, e podem ser controlados remotamente via dashboard web.

**Inspirado no** [Dasai Mochi](https://github.com/nichelaboratory) — adaptado para ESP32-WROOM com hardware acessível.

### ✨ Destaques

- **👆 Sensor Touch** — Toque para animar, segure para ver informações, toque duplo para mudar humor
- **🎭 12 Expressões** — 6 animações built-in + 6 expressões especiais com LED e som
- **🎲 Auto Expressões** — Animações aleatórias a cada 15-30 segundos
- **⏰ Relógio** — Sincronizado automaticamente pelo navegador
- **📺 Menu OLED** — 5 telas informativas (WiFi, sensores, relógio, sistema, sobre)
- **🌐 Dashboard Web** — Controle total via celular ou PC
- **🖼️ 3 Temas de Splash** — Minimal, Matrix e Wave

---

## 🚀 Quick Start

```bash
# 1. Compilar e carregar
pio run -t upload

# 2. Conectar no WiFi
#    SSID: RoboEyes
#    Senha: roboeyes123

# 3. Abrir o dashboard
#    http://192.168.4.1
```

---

## 📌 Hardware

| Componente | Modelo | Função |
|:---|:---|:---|
| Microcontrolador | ESP32-WROOM-32 (HW-724) | Processamento + WiFi |
| Display | OLED 128x64 SSD1306 bicolor | Olhos animados |
| Sensor Temp/Umidade | HTU21D (GY-21) | Controle de humor |
| Acelerômetro | BMI160 (GY-BMI160) | Eye tracking por inclinação |
| Buzzer | Passivo 5V | Feedback sonoro |
| LED RGB | KY-009 | Indicação visual de humor |
| Touch | Fio/placa cobre no GPIO 15 | Interação por toque |

---

## 🔌 Pinagem

### Barramento I2C Unificado (Wire)

Todos os dispositivos I2C compartilham o **mesmo barramento**:

| GPIO | Função | Dispositivos |
|:---|:---|:---|
| **5** | SDA | OLED + HTU21D + BMI160 |
| **4** | SCL | OLED + HTU21D + BMI160 |
| **16** | RST | OLED (Reset) |

### Periféricos

| GPIO | Função | Componente |
|:---|:---|:---|
| **0** | PWM | Buzzer Passivo |
| **13** | PWM | LED RGB — Red |
| **12** | PWM | LED RGB — Green |
| **14** | PWM | LED RGB — Blue |
| **15** | Touch3 | Sensor Capacitivo |
| **25** | — | 🟢 Livre |
| **26** | — | 🟢 Livre |

### Alimentação

| Pino | Tensão | Dispositivos |
|:---|:---|:---|
| 3V3 | 3.3V | HTU21D, BMI160 |
| GND | GND | Todos |
| 5V/Vin | 5V | Buzzer, alimentação USB |

---

## 🔧 Diagrama de Ligação

```
    ┌─────────────────────────────────────┐
    │         ESP32-WROOM (HW-724)        │
    │           ┌─────────┐               │
    │           │  OLED   │               │
    │           │ 128x64  │               │
    │           └─────────┘               │
    │                                     │
    │  GPIO 5 (SDA) ──┬── HTU21D SDA      │
    │                 └── BMI160 SDA      │
    │  GPIO 4 (SCL) ──┬── HTU21D SCL      │
    │                 └── BMI160 SCL      │
    │  3V3 ───────────┬── HTU21D VCC      │
    │                 └── BMI160 VCC      │
    │  GND ───────────┬── HTU21D GND      │
    │                 ├── BMI160 GND      │
    │                 ├── Buzzer GND      │
    │                 └── LED RGB GND     │
    │                                     │
    │  GPIO 0  ──── Buzzer Signal         │
    │  GPIO 13 ──── LED RGB Red           │
    │  GPIO 12 ──── LED RGB Green         │
    │  GPIO 14 ──── LED RGB Blue          │
    │  GPIO 15 ──── Touch Pad / Wire      │
    │                                     │
    │  GPIO 25 ──── (livre)               │
    │  GPIO 26 ──── (livre)               │
    └─────────────────────────────────────┘
```

> **💡 Dica:** Para o sensor touch, basta soldar um fio ou uma placa de cobre no GPIO 15. Ao tocar com o dedo, o ESP32 detecta a mudança de capacitância.

---

## 👆 Sensor Touch — Gestos

| Gesto | Duração | Ação |
|:---|:---|:---|
| **Toque curto** | < 300ms | Expressão aleatória (1 de 12) |
| **Toque longo** | > 1 segundo | Abre o menu OLED |
| **Toque duplo** | 2x em < 400ms | Cicla humor (Happy → Angry → Tired → Default) |

---

## 🎭 Expressões

### Animações Built-in (RoboEyes)

| Ação | Descrição |
|:---|:---|
| `blink` | Piscar os dois olhos |
| `confused` | Olhos confusos girando |
| `laugh` | Olhos de risada |
| `wink_l` / `wink_r` | Piscar esquerdo / direito |
| `cyclops` | Modo ciclope (um olho só) |

### Expressões Especiais (v5.0)

| Expressão | Duração | LED | Efeito Sonoro |
|:---|:---|:---|:---|
| 😍 **Love** | 2s | 🩷 Rosa | Notas ascendentes |
| 😱 **Scared** | 1.5s | 💛 Amarelo | Bips agudos |
| 🤨 **Suspicious** | 3s | 🟠 Laranja | Tom grave |
| 😪 **Sleepy** | 2.5s | 🩵 Azul claro | Nota longa |
| 🤩 **Excited** | 2s | 🌈 Arco-íris | Trinca rápida |
| 😵‍💫 **Dizzy** | 2.5s | 💜 Roxo | Notas alternadas |

### 🎲 Auto Expressões

Quando ativado, a cada **15-30 segundos** uma expressão aleatória é executada automaticamente, trazendo vida ao robô.

---

## 📺 Menu OLED

5 telas informativas com indicador de página (bolinhas), ciclo automático de 3s cada:

| # | Tela | Informações |
|:--|:-----|:------------|
| 1 | 📡 **WiFi** | SSID, senha, IP, clientes conectados |
| 2 | 🌡️ **Sensores** | Temperatura, umidade, acelerômetro, humor |
| 3 | ⏰ **Relógio** | Hora e data (sincronizados do navegador) |
| 4 | ⚙️ **Sistema** | Uptime, RAM livre, Flash, status dos sensores, valor touch |
| 5 | 🤖 **Sobre** | Versão, placa, autor |

---

## 🌡️ Sistema de Humor Automático

| Temperatura | Humor | LED | Efeito |
|:---|:---|:---|:---|
| > 30°C | 😠 **ANGRY** | 🔴 Vermelho | Suor ativado |
| < 18°C | 😴 **TIRED** | 🔵 Azul | — |
| 18–30°C | 😊 **HAPPY** | 🟢 Verde | — |

O humor também pode ser alterado manualmente via **dashboard** ou **toque duplo**.

---

## 🖼️ Temas de Intro (Splash Screen)

| Tema | Descrição |
|:---|:---|
| ✨ **Minimal** | Texto simples com info do WiFi |
| 💚 **Matrix** | Chuva de caracteres estilo Matrix |
| 🌊 **Wave** | Animação de ondas sinusoidais |

Selecionável pelo dashboard. Aplica-se no próximo reboot.

---

## 🌐 Dashboard Web

Interface premium com design dark glassmorphism, acessível em `http://192.168.4.1`.

### Funcionalidades

- 📊 **Sensores** em tempo real (temp, umidade, acelerômetro, touch)
- 🎛️ **10 Toggles** para controlar todas as features
- 😄 **4 Humores** selecionáveis
- 🎬 **6 Animações** básicas + **6 Expressões** especiais
- ⚙️ **Calibração** de sensibilidade (sliders)
- 📐 **Forma** dos olhos customizável
- 🖼️ **Seletor** de tema de splash
- ⏰ **Relógio** sincronizado automaticamente
- 📺 **Botão** para mostrar info no OLED
- 🏷️ **Badge** de modo atual (Olhos / Menu / Animação)

---

## 🌐 API REST

### `GET /api/status`

Retorna estado completo do sistema.

```json
{
  "temp": 24.5, "hum": 62,
  "ax": 0.12, "ay": -0.05, "az": 0.98,
  "mood": 1, "bmi": true, "htu": true,
  "mode": 0, "touch": 45, "clockSynced": true,
  "ch": 14, "cm": 30, "cs": 0,
  "t": {
    "tracking": true, "automood": true,
    "buzzer": true, "led": true,
    "blinker": true, "idle": false,
    "sweat": false, "curiosity": true,
    "autoExpr": true, "touch": true
  }
}
```

### `POST /api/toggle`

```json
{ "feature": "autoExpr", "state": true }
```

Features: `tracking`, `automood`, `buzzer`, `led`, `blinker`, `idle`, `sweat`, `curiosity`, `autoExpr`, `touch`

### `POST /api/mood`

```json
{ "mood": "happy" }
```

Valores: `happy`, `angry`, `tired`, `default`

### `POST /api/eyes`

```json
{ "action": "love" }
```

Ações: `blink`, `confused`, `laugh`, `wink_l`, `wink_r`, `cyclops`, `love`, `scared`, `suspicious`, `sleepy`, `excited`, `dizzy`

### `POST /api/screen`

```json
{ "action": "menu" }
```

Ações: `menu`, `eyes`

### `POST /api/time`

Sincroniza o relógio interno.

```json
{ "h": 14, "m": 30, "s": 0, "d": 13, "mo": 2, "y": 2026 }
```

### `POST /api/splash`

```json
{ "theme": 1 }
```

Temas: `0` = Minimal, `1` = Matrix, `2` = Wave

### `POST /api/calibrate`

```json
{ "threshold": 0.3, "shakeThreshold": 1.5 }
```

### `POST /api/shape`

```json
{ "w": 36, "h": 36, "r": 8, "s": 10 }
```

---

## 🎛️ Features Toggles

| Feature | Descrição | Padrão |
|:---|:---|:---|
| 👀 **Eye Tracking** | Olhos seguem inclinação (BMI160) | ✅ ON |
| 🌡️ **Auto Mood** | Humor baseado na temperatura | ✅ ON |
| 🔊 **Buzzer** | Feedback sonoro | ✅ ON |
| 💡 **LED RGB** | Cor indica o humor | ✅ ON |
| 😉 **Auto Blinker** | Pisca automático | ✅ ON |
| 🔄 **Idle Mode** | Olhos se movem sozinhos | ❌ OFF |
| 💧 **Sweat** | Efeito gotas de suor | ❌ OFF |
| 🧐 **Curiosity** | Olhos crescem ao olhar de lado | ✅ ON |
| 🎭 **Auto Expressões** | Expressões aleatórias | ✅ ON |
| 👆 **Touch Sensor** | Interação por toque | ✅ ON |

---

## 📁 Estrutura do Projeto

```
esp32_roboeyes/
├── platformio.ini          # Configuração PlatformIO
├── README.md               # Esta documentação
├── docs/
│   └── wiring_diagram.png  # Diagrama de ligação
└── src/
    ├── main.cpp            # Firmware principal (~730 linhas)
    └── dashboard.h         # Dashboard HTML embarcado (PROGMEM)
```

---

## 📦 Dependências

| Biblioteca | Versão | Função |
|:---|:---|:---|
| Adafruit SSD1306 | ^2.5.13 | Driver OLED |
| Adafruit GFX | ^1.11.11 | Gráficos 2D |
| FluxGarage RoboEyes | ^1.1.1 | Animação de olhos |
| Adafruit HTU21DF | ^1.0.6 | Sensor temp/umidade |
| Async TCP | ^3.1.4 | TCP assíncrono |
| ESPAsyncWebServer | ^3.6.0 | Web server |
| ArduinoJson | ^7.3.0 | Parsing JSON |

---

## 🏗️ Arquitetura

```
┌──────────────────────────────────────────────────────────┐
│                    ESP32-WROOM v5.0                       │
│                                                           │
│  ┌──────────┐  ┌──────────┐  ┌────────────────────────┐ │
│  │ WiFi AP  │  │ RoboEyes │  │   AsyncWebServer :80   │ │
│  │ RoboEyes │  │ 12 expr  │  │                        │ │
│  │ .4.1     │  │ OLED     │  │  /api/status  (GET)    │ │
│  └──────────┘  └────┬─────┘  │  /api/toggle  (POST)   │ │
│                     │        │  /api/mood    (POST)    │ │
│  ┌──────┬──────┐   │        │  /api/eyes    (POST)    │ │
│  │BMI160│HTU21D│◄──┘        │  /api/screen  (POST)    │ │
│  │ 0x69 │ 0x40 │ I2C       │  /api/time    (POST)    │ │
│  │      │      │ GPIO 5/4  │  /api/splash  (POST)    │ │
│  └──────┴──────┘            │  /api/calibrate (POST)  │ │
│                              │  /api/shape   (POST)    │ │
│  ┌────────┐  ┌─────┐       └────────────────────────┘ │
│  │ Buzzer │  │Touch│  ┌─────┐  ┌─────┐                │
│  │ GPIO 0 │  │GP 15│  │Clock│  │ LED │                │
│  │ Melodia│  │Gestos│  │Sync │  │ RGB │                │
│  └────────┘  └─────┘  └─────┘  └─────┘                │
└──────────────────────────────────────────────────────────┘
```

---

## ⚠️ Notas Técnicas

- **BMI160:** Usa leitura direta de registradores I2C (sem biblioteca DFRobot) para evitar conflitos com o OLED.
- **Macro DEFAULT:** É desfeita (`#undef`) antes de incluir `FluxGarage_RoboEyes.h` por conflito com `Arduino.h`.
- **Detecção automática:** Se o BMI160 ou HTU21D não estiverem conectados, as features correspondentes são desabilitadas automaticamente.
- **Touch calibração:** O limiar padrão é `30`. Valores abaixo indicam toque. Ajuste `TOUCH_THRESHOLD` se necessário.
- **Relógio:** Precisa de conexão inicial ao dashboard para sincronizar. Mantém precisão por software (drift ~1s/hora).
- **Firmware:** ~730 linhas, ~898KB Flash, ~45KB RAM.

---

## 🔄 Changelog

| Versão | Mudanças |
|:---|:---|
| **v5.0** | Touch sensor, auto expressões, relógio, melodias temáticas, 3 splash screens |
| **v4.0** | Menu OLED (4 telas), 6 expressões especiais, splash screen |
| **v3.0** | Dashboard web, REST API, feature toggles |
| **v2.0** | BMI160 eye tracking, HTU21D mood, buzzer, LED RGB |
| **v1.0** | RoboEyes básico com OLED |

---

## 👨‍💻 Autor

Desenvolvido por **[@raunick](https://github.com/raunick)**

---

## 📜 Licença

MIT License — Projeto educacional. Bibliotecas de terceiros sob suas respectivas licenças.
