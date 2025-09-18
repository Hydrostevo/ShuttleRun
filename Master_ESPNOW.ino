// Master_ESPNOW.ino

//================================================
// --- ESP32 MAC addresses ---
//*** Replace with your devices ***//
//  {0x??,0x??,0x??,0x??,0x??,0x??},  - Master
//  {0x??,0x??,0x??,0x??,0x??,0x??},  - Slave 2
//  {0x??,0x??,0x??,0x??,0x??,0x??},  - Slave 3
//  {0x??,0x??,0x??,0x??,0x??,0x??},  - Slave 4
//  {0x??,0x??,0x??,0x??,0x??,0x??}   - Slave 5
//================================================

#include <Arduino.h>
#include <FastLED.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "webpage.h"

AsyncWebServer server(80);

// ---------------- LED setup ----------------
#define LED_PIN     5
#define BUTTON1_PIN 18  // sequence/start button
#define BUTTON2_PIN 19  // reset button
#define NUM_LEDS    8
CRGB leds[NUM_LEDS];

// ---------------- Game parameters ----------------
const unsigned long TIMER_DURATION = 20000UL; // 20 seconds
const int SLAVE_COUNT = 2; // adjust to how many Slaves you have

enum State { IDLE, RUNNING, SUCCESS, FAILED };
State currentState = IDLE;

unsigned long startTime = 0;
bool slavePressed[10] = {false};   // supports up to 10 Slaves
bool slaveError[10] = {false};     // track slave error states
unsigned long completionTime = 0;  // track completion time
int pressesCount = 0;
bool sequenceCompleted = false;

// ---------------- ESP-NOW message struct ----------------
typedef struct struct_message {
  uint8_t fromID;
  char cmd[16];
} struct_message;

struct_message incomingMessage;

// ---------------- Slaves’ MAC addresses ----------------
// Replace with your Slaves' real MACs
uint8_t slave2Mac[6] = {0x08,0xB6,0x1F,0x29,0xA2,0xB8};  // Slave 2 [Naked AZ Delivery]
uint8_t slave3Mac[6] = {0x10,0x52,0x1C,0x5B,0x46,0x6C};  // Slave 3 [Heltec]
uint8_t *slaveMacs[SLAVE_COUNT] = { slave2Mac, slave3Mac };

// ---------------- ESP-NOW callbacks ----------------
void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));
  Serial.printf("[RECV] From %d: %s\n", incomingMessage.fromID, incomingMessage.cmd);

  if (strcmp(incomingMessage.cmd, "PRESSED") == 0) {
     int id=incomingMessage.fromID;
    if (!slavePressed[incomingMessage.fromID]) {
      slavePressed[incomingMessage.fromID] = true;
      pressesCount++;

      // mark progress as purple → will turn green later
      for (int i = 0; i < NUM_LEDS; i++) {
        if (leds[i] == CRGB::Blue || leds[i] == CRGB::Black) {
          leds[i] = CRGB::Purple;
          break;
        }
      }

      FastLED.show();
      Serial.printf("[GAME] Slave %d pressed, progress %d/%d\n", 
                    incomingMessage.fromID, pressesCount, SLAVE_COUNT);

      if (pressesCount == SLAVE_COUNT && currentState == RUNNING) {
        Serial.println("[GAME] All Slaves pressed, waiting for final Master press...");
      }
    }
  }
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("[SEND_CB] Delivery: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ---------------- Helpers ----------------
void addPeer(uint8_t *mac){
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[ERROR] Failed to add peer");
  } else {
    Serial.printf("[PEER] Added: %02X:%02X:%02X:%02X:%02X:%02X\n",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }
}

void sendAll(const char *cmd){
  struct_message msg;
  msg.fromID = 1; // master
  strcpy(msg.cmd, cmd);
  for (int i=0;i<SLAVE_COUNT;i++){
    esp_err_t result = esp_now_send(slaveMacs[i], (uint8_t*)&msg, sizeof(msg));
    Serial.printf("[SEND] To Slave %d: %s (%s)\n", i+2, cmd,
                  result == ESP_OK ? "queued" : "error");
  }
}

void setAll(CRGB c){ fill_solid(leds,NUM_LEDS,c); FastLED.show(); }

void startSequence(){
  memset(slavePressed,false,sizeof(slavePressed));
  pressesCount = 0;
  sequenceCompleted = false;
  startTime = millis();
  currentState = RUNNING;
  sendAll("START");
  Serial.println("[GAME] Sequence started!");
}

void resetGame(){
  memset(slavePressed,false,sizeof(slavePressed));
  pressesCount = 0;
  sequenceCompleted = false;
  currentState = IDLE;
  setAll(CRGB::Blue);
  sendAll("RESET");
  completionTime = 0;
  Serial.println("[GAME] Reset to idle.");
}

void endAsSuccess(){
  Serial.println("[GAME] SUCCESS!");
  for (int k=0;k<6;k++){ setAll(k%2?CRGB::Black:CRGB::Green); delay(200);}
  setAll(CRGB::Green);
  sendAll("SUCCESS");
  sequenceCompleted = true;
  completionTime = millis() - startTime;
  currentState = SUCCESS;
}

void endAsFail(){
  Serial.println("[GAME] FAIL!");
  for (int k=0;k<6;k++){ setAll(k%2?CRGB::Black:CRGB::Red); delay(200);}
  setAll(CRGB::Red);
  sendAll("FAIL");
  sequenceCompleted = true;
  completionTime = millis() - startTime;
  currentState = FAILED;
}

bool readButton1(){ return digitalRead(BUTTON1_PIN)==LOW; }
bool readButton2(){ return digitalRead(BUTTON2_PIN)==LOW; }

// --------------- Weppage ---------------
String getWebPage() {

  return String(index_html);
}
// ---------------- Setup ----------------
void setup(){
  Serial.begin(115200);
  pinMode(BUTTON1_PIN,INPUT_PULLUP);
  pinMode(BUTTON2_PIN,INPUT_PULLUP);
  FastLED.addLeds<WS2812,LED_PIN,GRB>(leds,NUM_LEDS);
  FastLED.setBrightness(80);
  setAll(CRGB::Blue);

  WiFi.mode(WIFI_AP_STA);
  
  // Set WiFi channel to 1 for ESP-NOW compatibility
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  
  if (esp_now_init() != ESP_OK){ Serial.println("[ERROR] ESP-NOW init failed"); return; }
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  // add Slaves
  for (int i=0;i<SLAVE_COUNT;i++) addPeer(slaveMacs[i]);

  // Setup AP on same channel as ESP-NOW (channel 1)
WiFi.softAP("ShuttleRun", "12345678", 1);
Serial.println("AP IP: " + WiFi.softAPIP().toString());

// Web server routes
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(200, "text/html", getWebPage());
});

server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<512> doc;
    const char* stateStr;
    switch(currentState){
      case IDLE: stateStr="IDLE"; break;
      case RUNNING: stateStr="RUNNING"; break;
      case SUCCESS: stateStr="SUCCESS"; break;
      case FAILED: stateStr="FAILED"; break;
      default: stateStr="IDLE"; break;
    }
    doc["state"] = stateStr;
    doc["timer"] = (currentState==RUNNING) ?
        max(0,(int)((TIMER_DURATION-(millis()-startTime))/1000)) : 0;
    doc["completionTime"] = (completionTime>0) ? (completionTime/1000.0) : 0; // seconds

    JsonObject slaves=doc.createNestedObject("slavesState");
    for(int i=2;i<=SLAVE_COUNT+1;i++){
      if(slaveError[i]) slaves[String(i)]="error";
      else if(slavePressed[i]) slaves[String(i)]="pressed";
      else slaves[String(i)]="waiting";
    }
    String json; serializeJson(doc,json);
    request->send(200,"application/json",json);
});

server.begin();
}

// ---------------- Loop ----------------
void loop(){
  if (readButton2()){ // reset button
    resetGame();
    delay(300);
  }

  if (readButton1()){
    if (currentState == IDLE) {
      startSequence();
    } else if (sequenceCompleted) {
      resetGame();
    } else {
      // Master press during sequence
      Serial.println("[GAME] Master pressed.");
      if (pressesCount == SLAVE_COUNT) {
        endAsSuccess();
      }
    }
    delay(300);
  }

  if (currentState == RUNNING){
    unsigned long elapsed = millis() - startTime;

    // --- countdown layer ---
    int ledsRemaining = map(elapsed, 0, TIMER_DURATION, NUM_LEDS, 0);
    for (int i=0; i<NUM_LEDS; i++) {
      leds[i] = (i < ledsRemaining) ? CRGB::Blue : CRGB::Black;
    }

    // --- overlay progress (purple / green) ---
    for (int i=0;i<NUM_LEDS;i++){
      if (leds[i] == CRGB::Purple || leds[i] == CRGB::Green) {
        // keep progress
      }
    }

    FastLED.show();

    if (elapsed >= TIMER_DURATION){
      if (pressesCount == SLAVE_COUNT) endAsSuccess();
      else endAsFail();
    }
  }
}
