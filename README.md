<p align="center">
  <img src="https://img.shields.io/badge/ESP8266-Based-blue?style=for-the-badge&logo=espressif&logoColor=white" alt="ESP8266" />
  <img src="https://img.shields.io/badge/ESP--NOW-Wireless-00d4aa?style=for-the-badge" alt="ESP-NOW" />
  <img src="https://img.shields.io/badge/Version-3.3-green?style=for-the-badge" alt="Version" />
  <img src="https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge" alt="License" />
</p>

<h1 align="center">🌐 ESP-NOW Multi-Slave Relay Control System</h1>

<p align="center">
  <strong>Sistema de Automação IoT de Alto Desempenho com ESP-NOW</strong><br>
  Controle independente de múltiplos módulos relés via ESP-12F Master com interface web moderna e display OLED.
</p>

---

## 📖 Sumário

- [Visão Geral](#-visão-geral)
- [Características](#-características)
- [Arquitetura do Sistema](#-arquitetura-do-sistema)
- [Hardware Necessário](#-hardware-necessário)
- [Pinout & Conexões](#-pinout--conexões)
- [Instalação](#-instalação)
- [Configuração](#-configuração)
- [Uso](#-uso)
- [Interface Web](#-interface-web)
- [Protocolo de Comunicação](#-protocolo-de-comunicação)
- [Troubleshooting](#-troubleshooting)
- [Roadmap](#-roadmap)
- [Licença](#-licença)

---

## 🔭 Visão Geral

Este projeto implementa um **sistema de controle de relés distribuído** utilizando a comunicação **ESP-NOW** da Espressif, permitindo controlar múltiplos dispositivos ESP-01S (Slaves) a partir de um módulo central ESP-12F (Master).

### O Problema
Sistemas tradicionais de automação residencial dependem de infraestrutura de rede complexa (MQTT, cloud servers) ou possuem alta latência de resposta.

### A Solução
Utilizando o protocolo **ESP-NOW**, conseguimos:
- ⚡ **Latência ultra-baixa** (~1ms)
- 📡 **Comunicação peer-to-peer** sem necessidade de roteador
- 🔋 **Baixo consumo de energia**
- 🔒 **Criptografia AES-128** (opcional)

---

## ✨ Características

### 🖥️ ESP-12F Master Controller
| Feature | Descrição |
|---------|-----------|
| **Dashboard Web** | Interface Glassmorphism com controles individuais |
| **OLED HUD** | Display 0.96" com status em tempo real |
| **GPS Tracking** | Telemetria via TinyGPS++ |
| **mDNS** | Acesso via `http://esp-12f.local` |
| **Multi-Slave** | Controle independente de até N dispositivos |

### 🔌 ESP-01S Slave Modules
| Feature | Descrição |
|---------|-----------|
| **Relé 5V** | Controle de cargas AC/DC |
| **Web Dashboard** | Interface Cyber-Tech individual |
| **ID Único** | Sistema de endereçamento por ID |
| **Status Feedback** | Resposta de estado para o Master |
| **LED Sync** | LED onboard sincronizado com relé |

---

## 🏗️ Arquitetura do Sistema

```
┌─────────────────────────────────────────────────────────────────────┐
│                         REDE WIFI LOCAL                             │
│                      (SSID: Configurável)                           │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                │ WiFi + WebServer
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        ESP-12F MASTER                               │
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐               │
│  │  OLED HUD   │   │  Web Server │   │  GPS Module │               │
│  │  (SSD1306)  │   │  (Port 80)  │   │ (TinyGPS++) │               │
│  └─────────────┘   └─────────────┘   └─────────────┘               │
│                                                                      │
│                    ESP-NOW Broadcast                                 │
│                    (0xFF:FF:FF:FF:FF:FF)                             │
└─────────────────────────────────┬─────────────────────────────────────┘
                                  │
           ┌──────────────────────┼──────────────────────┐
           │                      │                      │
           ▼                      ▼                      ▼
┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐
│  ESP-01S SLAVE   │   │  ESP-01S SLAVE   │   │  ESP-01S SLAVE   │
│     ID = 1       │   │     ID = 2       │   │     ID = N       │
├──────────────────┤   ├──────────────────┤   ├──────────────────┤
│  Relay + LED     │   │  Relay + LED     │   │  Relay + LED     │
│  Web Dashboard   │   │  Web Dashboard   │   │  Web Dashboard   │
└──────────────────┘   └──────────────────┘   └──────────────────┘
```

---

## 🛠️ Hardware Necessário

### Master (ESP-12F Board)
| Componente | Especificação | Quantidade |
|------------|---------------|------------|
| ESP-12F Dev Board | Com CH340C USB | 1 |
| Display OLED | 0.96" SSD1306 I2C | 1 |
| Módulo GPS | NEO-6M (opcional) | 1 |
| Cabos Jumper | Macho-Fêmea | N |

### Slave (ESP-01S + Relay)
| Componente | Especificação | Quantidade |
|------------|---------------|------------|
| ESP-01S | 1MB Flash | N |
| Relay Board V5.0 | Para ESP-01S | N |
| Programador | USB para ESP-01S | 1 |

---

## 📍 Pinout & Conexões

### ESP-12F Master
```
┌───────────────────────────────────────┐
│              ESP-12F                  │
├───────────────────────────────────────┤
│  GPIO 2  ──────► OLED SDA             │
│  GPIO 14 ──────► OLED SCL             │
│  GPIO 16 ──────► LED Node (Vermelho)  │
│  GPIO 4  ──────► LED Board (Azul)     │
│  GPIO 12 ──────► GPS RX               │
│  GPIO 13 ──────► GPS TX               │
│  USB     ──────► Serial Debug 115200  │
└───────────────────────────────────────┘
```

### ESP-01S Slave
```
┌───────────────────────────────────────┐
│              ESP-01S                  │
├───────────────────────────────────────┤
│  GPIO 0  ──────► RELAY (Active LOW)   │
│  GPIO 2  ──────► LED Builtin (Sync)   │
│  VCC     ──────► 3.3V                 │
│  GND     ──────► GND                  │
└───────────────────────────────────────┘
```

---

## 📥 Instalação

### Pré-requisitos
- [PlatformIO](https://platformio.org/) (VSCode Extension ou CLI)
- Python 3.8+
- Driver CH340 (para ESP-12F)
- Driver USB-Serial (para ESP-01S)

### Clone o Repositório
```bash
git clone https://github.com/seu-usuario/esp-now-relay-system.git
cd esp-now-relay-system
```

### Instalação das Dependências

#### Para ESP-12F Master:
```bash
cd esp12f
pio lib install
```

#### Para ESP-01S Slave:
```bash
cd esp01s
pio lib install
```

---

## ⚙️ Configuração

### 1. Configurar WiFi
Edite as credenciais em ambos os arquivos `src/main.cpp`:

```cpp
const char *ssid = "NOME_DA_SUA_REDE";
const char *password = "SUA_SENHA";
```

### 2. Configurar ID dos Slaves
Para cada ESP-01S, defina um ID único antes de gravar:

```cpp
#define SLAVE_ID 1  // Mude para 2, 3, etc. para outras placas
```

### 3. Upload do Firmware

#### Master (ESP-12F):
```bash
cd esp12f
pio run -t upload
pio device monitor
```

#### Slave (ESP-01S):
```bash
cd esp01s
pio run -t upload
pio device monitor
```

> ⚠️ **Importante:** Ajuste a `upload_port` no `platformio.ini` conforme sua porta serial.

---

## 🎮 Uso

### Acessando o Dashboard Master
1. Ligue o ESP-12F e aguarde a conexão WiFi
2. Acesse via navegador: **http://esp-12f.local**
3. Ou verifique o IP no Monitor Serial

### Acessando Dashboards dos Slaves
Cada Slave possui seu próprio dashboard:
- **Slave 1**: http://esp-01s-1.local
- **Slave 2**: http://esp-01s-2.local

### Controle via Master
- **NODE LED**: Controla LED vermelho local
- **BOARD LED**: Controla LED azul local
- **SLAVE 01 / 02**: Envia comando ESP-NOW para relés remotos

---

## 🌐 Interface Web

### Master Dashboard (Cyber-Tech UI)
```
┌───────────────────────────────────────┐
│          CYBER-MASTER                 │
│       192.168.1.100 | -65dBm          │
├───────────────────────────────────────┤
│  LOCAL CONTROLS                       │
│  ┌─────────────┐ ┌─────────────┐     │
│  │  NODE LED   │ │  BOARD LED  │     │
│  │    [ON]     │ │    [OFF]    │     │
│  └─────────────┘ └─────────────┘     │
├───────────────────────────────────────┤
│  REMOTES (ESP-01S)                    │
│  Slave 01: ON     [OFF SLAVE 1]       │
│  Slave 02: OFF    [ON SLAVE 2]        │
├───────────────────────────────────────┤
│  GPS TELEMETRY                        │
│  🛰 SIGNAL LOCKED (8 SAT)             │
│  LAT: -23.550520                      │
│  LNG: -46.633308                      │
│        [OPEN IN GOOGLE MAPS]          │
└───────────────────────────────────────┘
```

### Slave Dashboard (Individual)
```
┌───────────────────────────────────────┐
│        CYBER-SLAVE #1                 │
│         192.168.1.101                 │
├───────────────────────────────────────┤
│  RELAY CONTROL                        │
│        [ ACTIVE ]                     │
│  ┌─────────────────────────────────┐ │
│  │       DEACTIVATE LOAD           │ │
│  └─────────────────────────────────┘ │
├───────────────────────────────────────┤
│  NODE TELEMETRY                       │
│  ID: SLAVE_1                          │
│  IP: 192.168.1.101                    │
│  MAC: AA:BB:CC:DD:EE:FF               │
│  MASTER LINK: 11:22:33:44:55:66       │
└───────────────────────────────────────┘
```

---

## 📡 Protocolo de Comunicação

### Estrutura de Comando (Master → Slave)
```cpp
typedef struct __attribute__((packed)) {
  int targetID;   // ID do Slave destino (1, 2, 3...)
  int command;    // 1 = Toggle
} struct_cmd;
```

### Estrutura de Status (Slave → Master)
```cpp
typedef struct __attribute__((packed)) {
  int senderID;     // ID do Slave que está respondendo
  bool relayState;  // Estado atual do relé
} struct_status;
```

### Fluxo de Comunicação
```
[Master]           [Broadcast]              [Slave 1]    [Slave 2]
    │                   │                        │            │
    │── CMD {id:1} ────►│                        │            │
    │                   │───────────────────────►│            │
    │                   │                        │ (PROCESSA) │
    │                   │◄──────────────────────│            │
    │◄── STATUS {1,ON} ─│                        │            │
    │                   │                        │            │
```

---

## 🔧 Troubleshooting

### ❌ Slave não responde aos comandos
1. Verifique se os IDs estão configurados corretamente
2. Confirme que ambos estão na mesma rede WiFi
3. Reinicie o Master e aguarde reconexão ESP-NOW

### ❌ Display OLED não funciona
1. Verifique as conexões I2C (SDA/SCL)
2. Confirme o endereço I2C (padrão: `0x3C`)
3. Teste com um I2C Scanner

### ❌ GPS não obtém fix
1. Aguarde 1-5 minutos em área aberta
2. Verifique conexões RX/TX (são cruzadas)
3. Confirme o baud rate (9600)

### ❌ LED do Slave não sincroniza
- O LED onboard (GPIO 2) é **Active LOW**
- `digitalWrite(LED_PIN, LOW)` = LED ON
- `digitalWrite(LED_PIN, HIGH)` = LED OFF

---

## 🗺️ Roadmap

- [x] v3.0 - Sistema Master/Slave básico
- [x] v3.1 - Correção de crashes ESP-NOW
- [x] v3.2 - OLED Async (Anti-Freeze)
- [x] v3.3 - Multi-Slave com IDs únicos + Cyber UI
- [ ] v3.4 - Criptografia AES-128
- [ ] v3.5 - OTA Updates
- [ ] v4.0 - Suporte a até 20 Slaves
- [ ] v4.1 - App Mobile (React Native)

---

## 📂 Estrutura do Projeto

```
projetos/
├── 📁 esp01s/               # Firmware do Slave
│   ├── 📁 src/
│   │   └── main.cpp         # Código principal Slave
│   ├── platformio.ini       # Configuração PlatformIO
│   └── DOCUMENTACAO_SLAVE.md
│
├── 📁 esp12f/               # Firmware do Master
│   ├── 📁 src/
│   │   └── main.cpp         # Código principal Master
│   ├── platformio.ini       # Configuração PlatformIO
│   ├── DOCUMENTACAO_MASTER.md
│   └── DOCUMENTACAO_PLACA.md
│
├── .gitignore
└── README.md                # Este arquivo
```

---

## 📜 Licença

Este projeto está licenciado sob a **MIT License** - veja o arquivo [LICENSE](LICENSE) para detalhes.

---

## 👤 Autor

Desenvolvido com ☕ e 💡 

---

<p align="center">
  <sub>Se este projeto te ajudou, considere dar uma ⭐ no repositório!</sub>
</p>
