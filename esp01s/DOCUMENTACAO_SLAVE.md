# 🔌 ESP-01S: SMART SLAVE UNIT (v3.3)

> **Módulo Relé Inteligente** controlado remotamente pelo Master (ESP-12F) via ESP-NOW, agora com suporte a múltiplos IDs e interface Cyber-Tech.

---

## 📋 Especificações do Sistema

- **Função**: Slave / Receiver / Actuator
- **Firmware**: v3.3 (Multi-Slave & Cyber-Tech)
- **Protocolo**: ESP-NOW + WiFi (Web Server)
- **Output**: Relé 5V (GPIO 0) + LED (GPIO 2) **Sincronizados**
- **Identidade**: Possui um `SLAVE_ID` único (1 ou 2)

## 📍 Pinout & Hardware

| Pino ESP | Função Hardware | Descrição |
| :--- | :--- | :--- |
| **GPIO 0** | **Relé** | Acionamento do Relé (Active Low - Relé V5.0) |
| **GPIO 2** | **LED Builtin** | LED Azul (Sincronizado com o Relé) |

---

## 🎮 Funcionalidades

O Slave hospeda um Web Server acessível via: **[http://esp-01s-X.local](http://esp-01s-1.local)** (onde X é o ID).

### 1. Comportamento do LED & Relé
- **Conectando WiFi**: LED azul pisca rápido (100ms).
- **Conectado**: LED azul acende por 3s e apaga.
- **Operação**: O LED azul da placa agora espelha **exatamente** o estado do relé.
    - **Relé Ativo** = LED Azul Aceso.
    - **Relé Inativo** = LED Azul Apagado.

### 2. Dashboard Web (Cyber-Tech)
Nova interface moderna com estilo industrial:
- **Tema**: Fundo escuro, fontes Roboto/Segoe e acentos em Neon Ciano.
- **Relay Control**: Botão grande com feedback visual de estado.
- **Telemetry**: Identificação clara do `ID` e do `MAC` do Master conectado.

### 3. ESP-NOW Targeted (Independente)
- **Filtro de ID**: O dispositivo agora só reage se o `targetID` enviado pelo Master for igual ao seu `SLAVE_ID`.
- **Feedback**: Envia seu próprio ID no pacote de status para o Master saber qual placa está respondendo.

---

## 🔧 Configuração de Múltiplos Dispositivos

Para usar mais de um relé, edite o código antes de gravar:
```cpp
#define SLAVE_ID 1 // Mude para 2, 3, etc. para outras placas
```

## 🚀 Como Iniciar

1. Grave o firmware com o ID desejado.
2. O Master automaticamente detectará a resposta do Slave no Dashboard.
3. Teste o acionamento independente pelo console do ESP-12F.

