# 🔌 ESP-01S: SMART SLAVE UNIT (v3.1)

> **Módulo Relé Inteligente** controlado remotamente pelo Master (ESP-12F) via ESP-NOW, com feedback via LED e Dashboard próprio.

---

## 📋 Especificações do Sistema

- **Função**: Slave / Receiver / Actuator
- **Firmware**: v3.1 (Bidirecional)
- **Protocolo**: ESP-NOW + WiFi (Web Server)
- **Output**: Relé 5V (GPIO 0) + LED (GPIO 2)

## 📍 Pinout & Hardware

| Pino ESP | Função Hardware | Descrição |
| :--- | :--- | :--- |
| **GPIO 0** | **Relé** | Acionamento do Relé (Active Low/High variável) |
| **GPIO 2** | **LED Builtin** | LED Azul bordo (Feedback Visual) |
| **RX/TX** | **Serial** | Debug e Gravação (Requer adaptador USB) |

---

## 🎮 Funcionalidades

O Slave hospeda um Web Server independente: **[http://esp-led.local](http://esp-led.local)**

### 1. Comportamento do LED
- **Conectando WiFi**: Pisca rápido (100ms) indicando busca de rede.
- **Conectado / Standby**: Espelha o estado do Relé.
    - **Relé ON** = LED ON.
    - **Relé OFF** = LED OFF.

### 2. Dashboard Web
Página de diagnóstico para verificar a saúde da conexão:
- **Network Info**: Mostra IP Local, MAC Local e **MAC do Master** (se pareado).
- **Status Link**: Mostra se recebeu comandos recentes do Master.
- **Botão Teste**: Permite ligar/desligar o relé localmente para testar o hardware.

### 3. ESP-NOW (Bidirecional)
- **Recebe**: Comandos de Toggle do Master.
- **Envia**: Confirmação de novo estado (ON/OFF) de volta para atualizar o OLED do Master.

---

## 🔧 Notas de Gravação (Upload)

O ESP-01S é sensível e requer um adaptador USB-Serial específico.

**Problema Comum**: `Device not configured` ou `Invalid Head of Packet`.
**Solução**:
1. Certifique-se que o pino GPIO0 está conectado ao GND durante o boot (Modo Flash).
2. Se o upload falhar repetidamente, **desplugue e plugue** o adaptador USB para resetar a porta Serial do Mac.
3. Velocidade de upload configurada para `115200` para maior estabilidade.

---

## 🚀 Como Iniciar

1. Ligue o ESP-01S na base do Relé (ou fonte 3.3V).
2. O LED azul piscará até conectar no WiFi "RAUL".
3. Uma vez fixo (ou apagado), está pronto.
4. Ao receber comando do Master, você ouvirá o "click" do relé.
