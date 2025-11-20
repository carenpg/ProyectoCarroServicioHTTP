

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>

// ================== WIFI ==================
const char* WIFI_SSID     = "Caren";
const char* WIFI_PASSWORD = "njks1234";

// ================== AWS IoT Core ==================
const char* AWS_IOT_ENDPOINT = "ar79nng0qgpza-ats.iot.us-east-2.amazonaws.com";
const uint16_t AWS_IOT_PORT = 8883;

const char* THING_NAME = "Esp32";
const char* CLIENT_ID = "esp32-car-sebas-001";

// Topics
const char* TOPIC_CMD       = "sebas/car/cmd";
const char* TOPIC_STATUS    = "sebas/car/status";
const char* TOPIC_DISTANCE  = "sebas/car/distance";
const char* TOPIC_IMU       = "sebas/car/imu";
const char* TOPIC_ALERT     = "sebas/car/alert";
const char* TOPIC_LWT       = "sebas/car/lwt";

// ================== CERTIFICADOS AWS ==================
// Amazon Root CA 1
static const char AWS_ROOT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

// Device Certificate
static const char DEVICE_CERTIFICATE[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUVISfs5zXmalydqr8Ckmiwki4KQwwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MTExODIyMTEw
NVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMUYGWqmn1dK3zOmuy2S
BxN9ZFrjhtYXu+wZxfZiR8NLU/ySbA3Kfn54yTNMVK3Lvv7hEaYSTm7aInO5Hco9
KqCfMXr9dh+M2lC9QjjGmT/RbuF9QLt6lzLhhGe8JLBpf4RDuwHxjddEIqaLOsLn
9fNZK79Xww5tvmEXEliBkjTmTo7rIr2lxap6ZXOoIxldQdkbojm8OUvdH3N55lVY
1P90LOxgShZl6EhWzz+C85jACAm42/sYVy9pvZ504TelvVuH2mMgnj3usiu4lR75
IIzBpIaPtwrK/o2FPndSf31YlwiJg2KEVJ+246/BE/08i7Sfhna4JBqw6pJwjIdn
qE8CAwEAAaNgMF4wHwYDVR0jBBgwFoAUqhI19XS2ytdhodZS3nkhOE4h+JEwHQYD
VR0OBBYEFMSWMvEji8YFm/WV7nWiNvbaV1ffMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBeatsLgqA0rOxhS+FdpQRSlbam
Eh32uUqe6hMaI8CrbNo3a+Xc9MEVG1UCIzsmmW5UhTFpDgtuc5ZOswTzvdFrhFoS
JUod/Njc2iPLjp8ZJZNrGKbMNGY0gpOg7z7fplyC94wSZRmsGyATL6wzhRPkV/YJ
dn04ma2puGwUL6SwsmVCsQsGX7vC2QBWvRu9nEjidBwv/stXEphf9rogEwfcbBFZ
haAnL6/9DSng9gmZPCK+2okYeGmP7yzgsYK2J9u2LN1uK0czvagVqKjFmt+EbRd2
5/HUe1kIiZX5WfnH5QLcLFvMFLoTBE48Rw+J+nEYHDy9Llr9Bqxkby1+PnyG
-----END CERTIFICATE-----
)EOF";

// Private Key - MANTENER EN SECRETO
static const char PRIVATE_KEY[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAxRgZaqafV0rfM6a7LZIHE31kWuOG1he77BnF9mJHw0tT/JJs
Dcp+fnjJM0xUrcu+/uERphJObtoic7kdyj0qoJ8xev12H4zaUL1COMaZP9Fu4X1A
u3qXMuGEZ7wksGl/hEO7AfGN10Qipos6wuf181krv1fDDm2+YRcSWIGSNOZOjusi
vaXFqnplc6gjGV1B2RuiObw5S90fc3nmVVjU/3Qs7GBKFmXoSFbPP4LzmMAICbjb
+xhXL2m9nnThN6W9W4faYyCePe6yK7iVHvkgjMGkho+3Csr+jYU+d1J/fViXCImD
YoRUn7bjr8ET/TyLtJ+GdrgkGrDqknCMh2eoTwIDAQABAoIBAHShxDimjVhcyCSK
n+1hoqpX45EdX2dF+EDJJ6qbkhfxdavbAfJPR/eThozCuMF39nhhu83ou11B3G1a
uFQ7SgRu8Br17nbRJUfuF0f9ziZDyzfIpQvMibjkVzQD1DpI09rzMbD+vo8KlO61
KXq5RYRR2l+KkbTyIFzYCFUzp8+BCE1SJICIXBDtUmAKECz0N/dVv3s2PMbRXJ41
xHXPDhn4o+ZhQ5yIPkBQN+r6pxBbAA7hjqtXQ4QOmFPHH2fqAMwojOxdfrdcbD7w
Bsffeb790evqE8PVa6fi1MNy2A5D7aqC1qItbMUJwwADiWKGl/qiqEmOHEpBD63w
oKukTiECgYEA5EuhQ8XJJccLU2ZhZ+ddiw+VkJkbH3RMvBU4K17T1LKejMBn7D31
BliYTQ6mCM0PEbM6yLgsRoaYpQYn/cFnsXg9nmYwYUtPBM8Jilt25h/rVH/RQRdR
ekL/8VzFlmYUxxvkBOJBoHKb54TBnRR6lTFEcgjP/Guzl7KfQXYGQ3cCgYEA3QMm
ekJGmzTXaRjMerJiv9VhiJWYIwlwtQMe7Oiv4cHTYdaf8IwlxW4BSGJ1iDJL+jhU
FSgVOSeKxDft6zEAdFPrqUgHVZ3b/+58H7CFxNTzdONZdFCKfBWC7wmgyGU5CjH0
IYM7Cle0ax56lx8maeP2vQ6cuET7VKZjBq4EB+kCgYBmovnllk5QhaQ54pV4OTR9
CIydbbgb9BVrmb6fAQsLXSKa9QXD2DcIm1wdiLBs1IQp2QJcqzB8pJaL2rCwDPup
df198UNe+pST/OC1K1nRLBiI7M4PDYS8CtM2mBbc/xfoTEm/SFlo4R3mgHDrRgG2
gWfcPMoFGgar9MpUi9NVEQKBgQCaM/5+wws8c0vwuirBWQFpU1ov0CBMIeQPh9Pl
/Bvrai8wTm1dios4Cx6+AGR82IsGVJLmCAd2z04Uxeksdg4ZHAyLGgR0CiZblWvp
OGK9CM+suIvLif51wRSP76nM6EM6B8yMLWHeP39UVfm2wzcsHrAjDLTwJMEvOMBT
AssxMQKBgCCZV3k68I35+ys7/NtiH5+5dfPgIevZrJgUreDtQhNGACRxp0RmmZgq
6ghdpeTd4C6D77j+FLbJRJcg5pneuQbHKkwQAZ8XKtCoUOklOgLHUYysoiEUZtbe
njiKbz50ojQquk9eoHN+V+R5cs3aV7kRNFtjnktIAc52FjnJ6oJh
-----END RSA PRIVATE KEY-----
)EOF";

// ================== L298N Pines ==================
#define IN1 27
#define IN2 26
#define IN3 25
#define IN4 33
#define ENA 32
#define ENB 14

// ================== HC-SR04 ==================
#define TRIG_PIN 4
#define ECHO_PIN 2
float SAFE_STOP_CM = 20.0f;

// ================== MPU6050 ==================
#define SDA_PIN 21
#define SCL_PIN 22
#define MPU_ADDR 0x68
#define G_CONST 9.80665f
const float ACC_LSB_PER_G    = 16384.0f;
const float GYRO_LSB_PER_DPS = 131.0f;

// ================== Estado ==================
int speedLeft  = 0;
int speedRight = 0;
bool moving = false;
String lastDir = "stop";
unsigned long moveEndMs = 0;
const unsigned long MAX_MOVE_MS = 5000;

WiFiClientSecure wifiClient;
PubSubClient mqtt(wifiClient);

// ================== Utils ==================
int clampi(int v, int lo, int hi){ return v<lo?lo:(v>hi?hi:v); }

void applySpeed(){
  analogWrite(ENA, clampi(speedLeft,  0, 255));
  analogWrite(ENB, clampi(speedRight, 0, 255));
}

void motorsStop(bool brake=true){
  if (brake){
    digitalWrite(IN1, HIGH); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, HIGH);
  }else{
    digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);
  }
  speedLeft = speedRight = 0;
  applySpeed();
  moving = false;
  lastDir = "stop";
}

void motorsForward(int pwm){
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  speedLeft = speedRight = clampi(pwm,0,255);
  applySpeed(); moving = true; lastDir = "forward";
}

void motorsBackward(int pwm){
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  speedLeft = speedRight = clampi(pwm,0,255);
  applySpeed(); moving = true; lastDir = "backward";
}

void motorsLeft(int pwm){
  int l = clampi(pwm-80, 0, 255);
  int r = clampi(pwm,     0, 255);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  speedLeft = l; speedRight = r; applySpeed(); moving = true; lastDir = "left";
}

void motorsRight(int pwm){
  int l = clampi(pwm,     0, 255);
  int r = clampi(pwm-80,  0, 255);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  speedLeft = l; speedRight = r; applySpeed(); moving = true; lastDir = "right";
}

void startMoveWithTimeout(const String& dir, int speedPct, float durationSec){
  int pwm = map(clampi(speedPct,0,100), 0,100, 0,255);
  unsigned long dur = (unsigned long)(durationSec * 1000.0f);
  if (dur == 0) dur = 500;
  if (dur > MAX_MOVE_MS) dur = MAX_MOVE_MS;

  if      (dir=="forward")   motorsForward(pwm);
  else if (dir=="backward")  motorsBackward(pwm);
  else if (dir=="left")      motorsLeft(pwm);
  else if (dir=="right")     motorsRight(pwm);
  else { motorsStop(); return; }

  moveEndMs = millis() + dur;
}

float readDistanceCm(float timeout_us = 25000.0f){
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long dur = pulseIn(ECHO_PIN, HIGH, (unsigned long)timeout_us);
  if (dur == 0) return -1.0f;
  return dur / 58.0f;
}

void publishStatus(){
  char buf[160];
  int sp = max(speedLeft, speedRight);
  snprintf(buf, sizeof(buf),
    "{\"moving\":%s,\"dir\":\"%s\",\"speed\":%d}",
    moving?"true":"false", lastDir.c_str(), sp);
  mqtt.publish(TOPIC_STATUS, buf, false);
}

void publishDistance(){
  float cm = readDistanceCm();
  bool obstacle = (cm >= 0 && cm < SAFE_STOP_CM);
  char b[128];
  if (cm < 0) {
    snprintf(b, sizeof(b), "{\"cm\":-1,\"obstacle\":false}");
  } else {
    snprintf(b, sizeof(b), "{\"cm\":%.1f,\"obstacle\":%s}", cm, obstacle?"true":"false");
  }
  mqtt.publish(TOPIC_DISTANCE, b, false);
}

void publishAlert(const char* type, float cm){
  char b[128];
  snprintf(b, sizeof(b), "{\"type\":\"%s\",\"cm\":%.1f}", type, cm);
  mqtt.publish(TOPIC_ALERT, b, false);
}

// ================== MPU6050 ==================
bool mpuReadRaw(int16_t& AcX,int16_t& AcY,int16_t& AcZ,int16_t& GyX,int16_t& GyY,int16_t& GyZ){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, 6, true) != 6) return false;
  AcX = (Wire.read()<<8) | Wire.read();
  AcY = (Wire.read()<<8) | Wire.read();
  AcZ = (Wire.read()<<8) | Wire.read();
  
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x43);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, 6, true) != 6) return false;
  GyX = (Wire.read()<<8) | Wire.read();
  GyY = (Wire.read()<<8) | Wire.read();
  GyZ = (Wire.read()<<8) | Wire.read();
  return true;
}

void publishIMU(){
  int16_t AcX,AcY,AcZ,GyX,GyY,GyZ;
  if (!mpuReadRaw(AcX,AcY,AcZ,GyX,GyY,GyZ)) return;

  float ax = (AcX / ACC_LSB_PER_G) * G_CONST;
  float ay = (AcY / ACC_LSB_PER_G) * G_CONST;
  float az = (AcZ / ACC_LSB_PER_G) * G_CONST;
  float gx = (GyX / GYRO_LSB_PER_DPS);
  float gy = (GyY / GYRO_LSB_PER_DPS);
  float gz = (GyZ / GYRO_LSB_PER_DPS);

  char b[192];
  snprintf(b, sizeof(b),
    "{\"ax\":%.3f,\"ay\":%.3f,\"az\":%.3f,\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f}",
    ax, ay, az, gx, gy, gz);
  mqtt.publish(TOPIC_IMU, b, false);
}

// ================== MQTT callback ==================
void mqttCallback(char* topic, byte* payload, unsigned int length){
  static char body[256];
  length = min(length, (unsigned int)sizeof(body)-1);
  memcpy(body, payload, length); body[length] = '\0';

  Serial.printf("📩 Comando: %s\n", body);

  String s = String(body);
  s.trim(); s.replace(" ", ""); s.replace("\n","");

  if (s.indexOf("\"stop\":true") >= 0 || s.indexOf("\"dir\":\"stop\"") >= 0){
    motorsStop();
    publishStatus();
    return;
  }

  int idx = s.indexOf("\"safe_stop_cm\":");
  if (idx >= 0){
    float newThr = s.substring(idx+15).toFloat();
    if (newThr > 0 && newThr < 2000) SAFE_STOP_CM = newThr;
    publishStatus();
    return;
  }

  String dir = "";
  idx = s.indexOf("\"dir\":\"");
  if (idx >= 0){
    int j = s.indexOf("\"", idx+7);
    if (j > idx) dir = s.substring(idx+7, j);
  }
  int speedPct = 50;
  idx = s.indexOf("\"speed\":");
  if (idx >= 0){
    speedPct = s.substring(idx+8).toInt();
  }
  float duration = 1.0f;
  idx = s.indexOf("\"duration\":");
  if (idx >= 0){
    duration = s.substring(idx+11).toFloat();
  }

  if (!(dir=="forward"||dir=="backward"||dir=="left"||dir=="right")){
    motorsStop();
    publishStatus();
    return;
  }
  speedPct = clampi(speedPct, 0, 100);
  if (duration <= 0) duration = 0.5f;

  if (dir=="forward"){
    float cm = readDistanceCm();
    if (cm >= 0 && cm < SAFE_STOP_CM){
      publishAlert("obstacle", cm);
      motorsStop();
      publishStatus();
      return;
    }
  }

  startMoveWithTimeout(dir, speedPct, duration);
  publishStatus();
}

// ================== AWS IoT ==================
void setupAWSIoT(){
  wifiClient.setCACert(AWS_ROOT_CA);
  wifiClient.setCertificate(DEVICE_CERTIFICATE);
  wifiClient.setPrivateKey(PRIVATE_KEY);
  
  mqtt.setServer(AWS_IOT_ENDPOINT, AWS_IOT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(60);
}

void ensureMqtt(){
  int attempts = 0;
  while (!mqtt.connected() && attempts < 3){
    attempts++;
    Serial.printf("🔌 Intento %d...\n", attempts);
    
    if (mqtt.connect(CLIENT_ID)){
      Serial.println(" ¡CONECTADO A AWS IoT!");
      Serial.println(" TLS + Certificados X.509");
      
      mqtt.publish(TOPIC_LWT, "{\"status\":\"online\"}", true);
      
      if (mqtt.subscribe(TOPIC_CMD)){
        Serial.printf(" Escuchando: %s\n\n", TOPIC_CMD);
      }
      return;
    } else {
      int state = mqtt.state();
      Serial.printf("Error rc=%d: ", state);
      
      switch(state){
        case -4: Serial.println("TIMEOUT"); break;
        case -3: Serial.println("CONNECTION_LOST"); break;
        case -2: Serial.println("CONNECT_FAILED"); break;
        case  5: Serial.println("NOT_AUTHORIZED - Verifica Policy"); break;
        default: Serial.println("ERROR"); break;
      }
      
      if (attempts < 3) delay(5000);
    }
  }
}

// ================== SETUP / LOOP ==================
unsigned long lastSenseMs = 0;
unsigned long lastTelemetryMs = 0;
unsigned long lastImuMs = 0;
unsigned long lastReconnectMs = 0;

void setup(){
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║   ESP32 Car - AWS IoT Secure     ║");
  Serial.println("╚═══════════════════════════════════╝");

  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  pinMode(ENA,OUTPUT); pinMode(ENB,OUTPUT);
  motorsStop(false);
  applySpeed();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0x00);
  Wire.endTransmission(true);

  Serial.println("📡 Conectando WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int wifi_attempts = 0;
  while(WiFi.status() != WL_CONNECTED && wifi_attempts < 20){ 
    delay(500); 
    Serial.print("."); 
    wifi_attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED){
    Serial.printf("\n✅ WiFi OK - IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n❌ WiFi ERROR");
    return;
  }

  Serial.printf("🌐 AWS: %s\n", AWS_IOT_ENDPOINT);
  Serial.printf("📦 Thing: %s\n\n", THING_NAME);
  
  setupAWSIoT();
  ensureMqtt();

  if (mqtt.connected()){
    publishStatus();
    Serial.println("✅ Sistema listo\n");
    Serial.println("Prueba desde AWS IoT → Test → MQTT test client");
    Serial.printf("Topic: %s\n", TOPIC_CMD);
    Serial.println("Mensaje: {\"dir\":\"forward\",\"speed\":50,\"duration\":2}\n");
  }
}

void loop(){
  if (WiFi.status() != WL_CONNECTED){
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while(WiFi.status() != WL_CONNECTED) delay(500);
  }
  
  if (!mqtt.connected()){
    unsigned long now = millis();
    if (now - lastReconnectMs > 10000){
      lastReconnectMs = now;
      ensureMqtt();
    }
  }
  
  mqtt.loop();

  if(moving && millis() >= moveEndMs){
    motorsStop();
    publishStatus();
  }

  if (moving && (millis() - lastSenseMs) >= 80){
    lastSenseMs = millis();
    float cm = readDistanceCm();
    if (cm >= 0 && cm < SAFE_STOP_CM){
      motorsStop();
      publishAlert("obstacle", cm);
      publishStatus();
    }
  }

  if ((millis() - lastTelemetryMs) >= 300){
    lastTelemetryMs = millis();
    publishDistance();
  }

  if ((millis() - lastImuMs) >= 200){
    lastImuMs = millis();
    publishIMU();
  }
}
