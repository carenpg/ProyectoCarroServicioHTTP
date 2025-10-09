#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ================== WIFI ==================
const char* WIFI_SSID     = "Caren";
const char* WIFI_PASSWORD = "njks1234";

// ================== L298N: Pines ==================
#define IN1 27
#define IN2 26
#define IN3 25
#define IN4 33
#define ENA 32
#define ENB 14

// ================== Estado y seguridad ==================
int speedLeft  = 200;  // 0..255
int speedRight = 200;  // 0..255
bool moving = false;
unsigned long moveEndMs = 0;
const unsigned long MAX_MOVE_MS = 5000; // máximo 5 s

WebServer server(80);

// ================== Utils ==================
int clampi(int v, int lo, int hi){ return v<lo?lo:(v>hi?hi:v); }

void applySpeed(){ 
  analogWrite(ENA, speedLeft); 
  analogWrite(ENB, speedRight); 
}

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

// ================== HTTP ==================
void handleHealth(){ 
  server.send(200,"application/json","{\"status\":\"ok\"}"); 
}

void handleMove(){
  if(!server.hasArg("dir") || !server.hasArg("speed") || !server.hasArg("duration")){
    server.send(400,"application/json","{\"error\":\"dir|speed|duration\"}"); 
    return;
  }

  String dir = server.arg("dir"); dir.toLowerCase();
  int speed = server.arg("speed").toInt();
  float duration = server.arg("duration").toFloat();

  startMoveWithTimeout(dir, speed, duration);

  String payload = "{\"dir\":\""+dir+"\",\"speed\":"+String(speed)+",\"duration\":"+String(duration)+"}";
  Serial.println("Comando HTTP recibido: " + payload);

  server.send(200,"application/json","{\"accepted\":true}");
}

// ================== SETUP / LOOP ==================
void setup(){
  Serial.begin(115200);

  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  pinMode(ENA,OUTPUT); pinMode(ENB,OUTPUT);
  motorsStop();

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status()!=WL_CONNECTED){ delay(300); Serial.print("."); }
  Serial.printf("\nWiFi OK. IP: %s\n", WiFi.localIP().toString().c_str());

  // Servidor HTTP
  server.on("/health", HTTP_GET, handleHealth);
  server.on("/move", HTTP_GET, handleMove);
  server.on("/move", HTTP_POST, handleMove);
  server.begin();
  Serial.println("Servidor HTTP listo. Usa /move para mandar comandos.");
}

void loop(){
  server.handleClient();

  // detener automáticamente cuando se cumpla el tiempo
  if(moving && millis() >= moveEndMs){
    motorsStop();
    Serial.println(">>> FIN DE ACCIÓN (auto-stop) <<<");
  }
}
