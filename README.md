# 🚗 ESP32 Car — Proyecto Completo  
Control vehicular mediante **ESP32 + IoT Core (MQTT + TLS)**  
Incluye **dashboard web funcional** para control y monitoreo en tiempo real.

---

## 📌 1. Descripción General

Este proyecto implementa un vehículo inteligente controlado remotamente mediante un **ESP32**, conectado de forma segura a **AWS IoT Core** usando **MQTT con certificados X.509**.

El sistema permite:

- Controlar los motores mediante **L298N**
- Medir distancia con **HC-SR04**
- Leer acelerómetro/giroscopio **MPU6050**
- Enviar telemetría a AWS
- Recibir comandos de movimiento
- Visualizar datos en un **dashboard web**
- Usar conexión **segura TLS**
- Toda la comunicación se hace por **MQTT**, no HTTP

---

# 📦 2. Librerías Utilizadas

### 🔧 Core
- **Arduino.h** → Funciones básicas del framework.

### 🌐 WiFi
- **WiFi.h**  
  - Conexión del ESP32 a la red WiFi.  
  - Obtención de IP, reconexiones básicas.

### 🔐 Seguridad/TLS
- **WiFiClientSecure.h**  
  - Permite conexiones seguras TLS/SSL.  
  - Carga certificados: Root CA, Certificado del dispositivo, Llave privada.  
  - Requerido para AWS IoT Core.

### 📨 MQTT
- **PubSubClient.h**  
  - Manejo de MQTT (publish/subscribe).  
  - Callback para mensajes entrantes.  
  - Trabaja encima de `WiFiClientSecure`.

### 🔌 I2C
- **Wire.h**  
  - Comunicación I2C con el MPU6050.

### 🧭 IMU (MPU6050)
- No usa librería externa.  
- Se manejan los registros internos:
  - 0x3B → acelerómetro
  - 0x43 → giroscopio

### 🎮 Motores L298N
Controlados por:
- `digitalWrite`
- `analogWrite` (PWM)

No se requiere librería adicional.

### 📏 Sensor HC-SR04
Implementado con:
- `digitalWrite`
- `pulseIn`


---

# 🗂️ 3. Estructura del Proyecto

# ProyectoCarroServicioHTTP

---

# 📨 4. API MQTT — Topics y Payloads

## 📥 Suscripción — comandos al ESP32

### **Topic principal:** `sebas/car/cmd`

#### Movimiento
```json
{
  "dir": "forward",
  "speed": 50,
  "duration": 2
}
{ "stop": true }
{ "safe_stop_cm": 30 }
 {
  "moving": true,
  "dir": "forward",
  "speed": 120
}
{
  "cm": 25.4,
  "obstacle": false
}
{
  "ax": 0.12,
  "ay": 9.80,
  "az": 0.03,
  "gx": 1.22,
  "gy": 0.03,
  "gz": -0.51
}
{
  "type": "obstacle",
  "cm": 12.5
}
{ "status": "online" }
}
## 5. Dashboard Web (YA INCLUIDO EN EL PROYECTO)

El dashboard permite:

Controlar el vehículo (adelante, atrás, izquierda, derecha)

Visualizar telemetría en vivo:

Distancia

Acelerómetro

Giroscopio

Estado del carro

Ver alertas del sistema

Conexión automática a MQTT por WSS (WebSockets + TLS)

Tecnologías del dashboard:

HTML5

CSS3

JavaScript

MQTT.js

Conexión WSS
6. Limitaciones Actuales

Reconexión WiFi simple
Sin backoff exponencial.

Parsing JSON manual
Recomendado migrar a ArduinoJson.

Sin control PID en motores
Los giros no son 100% precisos.

Movimiento limitado a 5 segundos
Definido por constante.

Sin validación profunda de payloads
JSON inválido puede romper la lógica.

MPU6050 sin calibración automática
Puede haber drift con el tiempo.

Sin logs en memoria
Todo es en vivo; no se guardan datos.
7. Estructura Interna del Firmware
setup()

Configurar GPIO

Iniciar WiFi

Configurar TLS

Cargar certificados

Conectar a AWS IoT

Suscribirse al topic

Publicar estado inicial

loop()

Mantener MQTT

Controlar motores

Leer distancia

Leer IMU

Detectar obstáculos

Publicar telemetría

Manejar timeout de movimiento

🚀 8. Futuras Mejoras
🔐 Seguridad

Backoff exponencial

Reconexión MQTT automática

Rotación de certificados X.509

🧠 Inteligencia

Control PID

Fusión de sensores

Mejor lógica de evitar colisiones

🔧 Hardware

Migrar HC-SR04 → VL53L0X

Agregar encoders en las ruedas

📊 Dashboard

Modo oscuro

Mapa del movimiento

Gráficas históricas

🧩 Código

Modularización completa

Migrar a ArduinoJson

Sistema de logging

📁 9. Documentación

Documentación en PDF incluida:

Documentación del Proyecto ESP32 Car.pdf

✔️ 10. Estado del Proyecto

🟢 Carro funcional
🟢 Dashboard funcional
🟢 Conexión MQTT con TLS estable
🔧 Mejoras futuras planificadas
