#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>

// ================== WIFI / MQTT ==================
const char* WIFI_SSID     = "Caren";
const char* WIFI_PASSWORD = "njks1234";

const char* MQTT_HOST = "broker.hivemq.com";
const int   MQTT_PORT = 1883;

const char* TOPIC_DIST = "carrito/hcsr04/dist";   // lecturas crudas
const char* TOPIC_PUB  = "carrito/status";        // estado general

WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);

// ================== HC-SR04 ==================
const int TRIG_PIN = 23;
const int ECHO_PIN = 22;
constexpr bool HCSR04_SIMULATED = false;  // cambia a true si no tienes el sensor
float lastDistanceCm = NAN;

float readHCSR04() {
  if (HCSR04_SIMULATED) {
    static bool seeded = false;
    if (!seeded) { randomSeed(esp_random()); seeded = true; }
    return 2.0f + (float)random(0, 39801) / 100.0f;  // 2–400 cm simulados
  } else {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
    if (duration == 0) return NAN;
    return (duration * 0.0343f) / 2.0f;  // distancia en cm
  }
}

// ================== MQTT ==================
void publishDistance() {
  lastDistanceCm = readHCSR04();
  String payload = String("{\"distancia_cm\":") + String(lastDistanceCm, 2) + "}";
  mqtt.publish(TOPIC_DIST, payload.c_str(), false);
  mqtt.publish(TOPIC_PUB, payload.c_str(), false);
  Serial.printf("MQTT -> %s %s\n", TOPIC_DIST, payload.c_str());
}

void ensureMqtt() {
  while (!mqtt.connected()) {
    String cid = "sensor-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if (mqtt.connect(cid.c_str())) {
      Serial.println("MQTT conectado ✅");
    } else {
      Serial.printf("MQTT rc=%d, reintento...\n", mqtt.state());
      delay(1000);
    }
  }
}

// ================== HTTP ==================
void handleHealth() { server.send(200, "application/json", "{\"status\":\"ok\"}"); }

void handleDistance() {
  float cm = readHCSR04();
  String payload = String("{\"distancia_cm\":") + String(cm, 2) + "}";
  server.send(200, "application/json", payload);
}

void handleDistancePublish() {
  publishDistance();
  String payload = String("{\"published\":true,\"distancia_cm\":") + String(lastDistanceCm, 2) + "}";
  server.send(200, "application/json", payload);
}

// ================== SETUP / LOOP ==================
void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.printf("\nWiFi OK. IP: %s\n", WiFi.localIP().toString().c_str());

  mqtt.setServer(MQTT_HOST, MQTT_PORT);

  server.on("/health", HTTP_GET, handleHealth);
  server.on("/distance", HTTP_GET, handleDistance);
  server.on("/distance/publish", HTTP_GET, handleDistancePublish);
  server.begin();
  Serial.println("HTTP listo: /distance y /distance/publish");
}

unsigned long lastPublishMs = 0;
const unsigned long PUBLISH_EVERY_MS = 2000;

void loop() {
  server.handleClient();

  if (!mqtt.connected()) ensureMqtt();
  mqtt.loop();

  unsigned long now = millis();
  if (now - lastPublishMs >= PUBLISH_EVERY_MS) {
    lastPublishMs = now;
    publishDistance();
  }
}
