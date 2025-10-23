#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>

// ================== WIFI / MQTT ==================
const char* WIFI_SSID     = "Caren";
const char* WIFI_PASSWORD = "njks1234";

const char* MQTT_HOST = "broker.hivemq.com";
const int   MQTT_PORT = 1883;

// Mantengo tu estructura de topics como const char*
const char* TOPIC_SUB = "carrito/control";        // comandos hacia el carrito
const char* TOPIC_PUB = "carrito/status";         // estado del carrito (incluye distancia)
const char* TOPIC_DIST = "carrito/hcsr04/dist";   // lecturas crudas de distancia (opcional)

WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);

// ================== L298N: Pines ==================
const int IN1 = 27;
const int IN2 = 26;
const int IN3 = 25;
const int IN4 = 33;
const int ENA = 32;
const int ENB = 14;

// ================== HC-SR04 (modo aleatorio activo) ==================
const int TRIG_PIN = 23;
const int ECHO_PIN = 22;         // válido en ESP32
constexpr bool HCSR04_SIMULATED = true;  // seguimos en modo aleatorio
float lastDistanceCm = NAN;

float readHCSR04() {
  if (HCSR04_SIMULATED) {
    // 2.00 .. 400.00 cm con 2 decimales
    // Semilla pseudoaleatoria basada en el RNG del ESP32 (solo una vez)
    static bool seeded = false;
    if (!seeded) { randomSeed(esp_random()); seeded = true; }
    float cm = 2.0f + (float)random(0, 39801) / 100.0f;
    return cm;
  } else {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
    if (duration == 0) return NAN;
    float cm = (duration * 0.0343f) / 2.0f;
    return cm;
  }
}

// ================== Estado y seguridad ==================
int speedLeft  = 200;  // 0..255
int speedRight = 200;  // 0..255
bool moving = false;
unsigned long moveEndMs = 0;
const unsigned long MAX_MOVE_MS = 5000;   // máximo 5 s

// Publicación periódica
const unsigned long PUBLISH_EVERY_MS = 2000;
unsigned long lastPublishMs = 0;

// ================== Utils ==================
int clampi(int v, int lo, int hi){ return v<lo?lo:(v>hi?hi:v); }
void applySpeed(){ analogWrite(ENA, speedLeft); analogWrite(ENB, speedRight); }

void motorsStop(){
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0); analogWrite(ENB, 0);
  moving = false;
}

void motorsForward(int pwm){
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  speedLeft = speedRight = clampi(pwm,0,255);
  applySpeed(); moving = true;
}

void motorsBackward(int pwm){
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  speedLeft = speedRight = clampi(pwm,0,255);
  applySpeed(); moving = true;
}

void motorsLeft(int pwm){
  int l = clampi(pwm-80, 0, 255);
  int r = clampi(pwm, 0, 255);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  speedLeft = l; speedRight = r; applySpeed(); moving = true;
}
void motorsRight(int pwm){
  int l = clampi(pwm, 0, 255);
  int r = clampi(pwm-80, 0, 255);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  speedLeft = l; speedRight = r; applySpeed(); moving = true;
}

void startMoveWithTimeout(const String& dir, int speedPct, float durationSec){
  int pwm = map(clampi(speedPct,0,100), 0,100, 0,255);
  unsigned long dur = (unsigned long)(durationSec * 1000.0f);
  if (dur == 0) dur = 500;
  if (dur > MAX_MOVE_MS) dur = MAX_MOVE_MS;

  if (dir=="forward")      motorsForward(pwm);
  else if (dir=="backward") motorsBackward(pwm);
  else if (dir=="left")     motorsLeft(pwm);
  else if (dir=="right")    motorsRight(pwm);
  else                      motorsStop();

  moveEndMs = millis() + dur;
}

// ================== MQTT ==================
void publishStatus(const String& msg){
  mqtt.publish(TOPIC_PUB, msg.c_str(), true);
}

void publishDistance(){   // publica en TOPIC_DIST y también en status
  lastDistanceCm = readHCSR04();
  String payload = String("{\"distancia_cm\":") + String(lastDistanceCm, 2) + "}";
  mqtt.publish(TOPIC_DIST, payload.c_str(), false);
  // además, incluir en el topic de estado general
  publishStatus(String("{\"hcsr04\":") + payload + "}");
  Serial.printf("MQTT -> %s %s\n", TOPIC_DIST, payload.c_str());
}

void onMqtt(char* topic, byte* payload, unsigned int len){
  String body; body.reserve(len);
  for(unsigned int i=0;i<len;i++) body += (char)payload[i];
  body.trim();
  Serial.printf("[MQTT %s] %s\n", topic, body.c_str());

  // comandos simples
  if (body=="F") { startMoveWithTimeout("forward", 60, 1.0); return; }
  if (body=="B") { startMoveWithTimeout("backward",60, 1.0); return; }
  if (body=="L") { startMoveWithTimeout("left",    60, 0.5); return; }
  if (body=="R") { startMoveWithTimeout("right",   60, 0.5); return; }
  if (body=="S") { motorsStop(); return; }

  // JSON {"dir":"forward","speed":60,"duration":2}
  body.toLowerCase();
  String dir; int speed=60; float dur=1.0;
  int p = body.indexOf("\"dir\"");
  if (p>=0){ int q = body.indexOf("\"", body.indexOf(":",p)+1); int r = body.indexOf("\"", q+1); if(q>0&&r>q) dir = body.substring(q+1,r); }
  p = body.indexOf("\"speed\"");
  if (p>=0){ speed = body.substring(body.indexOf(":",p)+1).toInt(); }
  p = body.indexOf("\"duration\"");
  if (p>=0){ dur = body.substring(body.indexOf(":",p)+1).toFloat(); }

  if (dir.length()) startMoveWithTimeout(dir, speed, dur);
}

void ensureMqtt(){
  while(!mqtt.connected()){
    String cid = "carrito-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if(mqtt.connect(cid.c_str())){
      mqtt.subscribe(TOPIC_SUB);
      Serial.println("MQTT conectado y suscrito a carrito/control");
    }else{
      Serial.printf("MQTT rc=%d, reintento...\n", mqtt.state());
      delay(1000);
    }
  }
}

// ================== HTTP (GET) ==================
void handleHealth(){ server.send(200,"application/json","{\"status\":\"ok\"}"); }

void handleMove(){
  if(!server.hasArg("dir") || !server.hasArg("speed") || !server.hasArg("duration")){
    server.send(400,"application/json","{\"error\":\"dir|speed|duration\"}"); return;
  }
  String dir = server.arg("dir"); dir.toLowerCase();
  int speed = server.arg("speed").toInt();
  float duration = server.arg("duration").toFloat();
  startMoveWithTimeout(dir, speed, duration);

  String payload = "{\"dir\":\""+dir+"\",\"speed\":"+String(speed)+",\"duration\":"+String(duration)+"}";
  publishStatus(payload);
  server.send(200,"application/json","{\"accepted\":true}");
}

void handleDistance(){ // GET /distance
  float cm = readHCSR04();
  String payload = String("{\"distancia_cm\":") + String(cm,2) + "}";
  server.send(200, "application/json", payload);
}

void handleDistancePublish(){ // GET /distance/publish
  publishDistance();
  String payload = String("{\"published\":true,\"distancia_cm\":") + String(lastDistanceCm,2) + "}";
  server.send(200, "application/json", payload);
}

// ================== SETUP / LOOP ==================
void setup(){
  Serial.begin(115200);

  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  pinMode(ENA,OUTPUT); pinMode(ENB,OUTPUT);
  motorsStop();

  // HC-SR04 (aunque estemos en simulado, dejamos pinMode listo)
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status()!=WL_CONNECTED){ delay(300); Serial.print("."); }
  Serial.printf("\nWiFi OK. IP: %s\n", WiFi.localIP().toString().c_str());

  // MQTT
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqtt);

  // HTTP
  server.on("/health", HTTP_GET, handleHealth);
  server.on("/move", HTTP_GET, handleMove);
  server.on("/move", HTTP_POST, handleMove);
  server.on("/distance", HTTP_GET, handleDistance);              // GET: devuelve lectura
  server.on("/distance/publish", HTTP_GET, handleDistancePublish);// GET: lee y publica
  server.begin();
  Serial.println("HTTP listo. Endpoints: /distance y /distance/publish");
}

void loop(){
  server.handleClient();

  if(!mqtt.connected()) ensureMqtt();
  mqtt.loop();

  // Auto-stop al vencer timeout
  if(moving && millis() >= moveEndMs){
    motorsStop();
    Serial.println(">>> FIN DE ACCION (auto-stop) <<<");
  }

  // Publicación periódica
  const unsigned long now = millis();
  if (now - lastPublishMs >= PUBLISH_EVERY_MS){
    lastPublishMs = now;
    publishDistance();  // publica lectura (simulada) cada 2 s
  }
}