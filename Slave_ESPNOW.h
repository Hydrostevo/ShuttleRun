// Slave_ESPNOW
#include <Arduino.h>
#include <FastLED.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

// ---------------- Change per board ----------------
#define SLAVE_ID   2   // Change to 2,3,4 etc. on each Slave
#define BUTTON_PIN 18
#define LED_PIN    5
#define NUM_LEDS   8

CRGB leds[NUM_LEDS];
bool sequenceRunning=false, pressedYet=false;
bool connectedToMaster=false;
//  Rainbow colours
uint8_t rainbowHue=0;
unsigned long lastRainbowUpdate=0;
//  Blue/Orange pulse colours
uint8_t pulseBrightness=0;
int8_t pulseDirection=1;
bool isBlue=true;
unsigned long lastPulseUpdate=0;

// ---------------- ESP-NOW message struct ----------------
typedef struct struct_message {
  uint8_t fromID;
  char cmd[16];
} struct_message;

struct_message incomingMessage;

// ---------------- Master MAC ----------------
// Replace with actual Master MAC
uint8_t masterMac[6] = {0x08,0xB6,0x1F,0x29,0xDF,0xCC};

// ---------------- Helpers ----------------
void setAll(CRGB c){
  fill_solid(leds,NUM_LEDS,c);
  FastLED.show(); }

void updateRainbow(){
  if (millis() - lastRainbowUpdate > 50) {
    fill_rainbow(leds, NUM_LEDS, rainbowHue, 32);
    FastLED.show();
    rainbowHue++;
    lastRainbowUpdate = millis();
  }
}

void updateBlueOrangePulse(){
  if (millis() - lastPulseUpdate > 15) {
    pulseBrightness += pulseDirection * 4;
    if (pulseBrightness >= 255) {
      pulseDirection = -1;
    } else if (pulseBrightness <= 0) {
             pulseDirection = -pulseDirection;
      isBlue = !isBlue; // Switch color when reaching minimum brightness
     }
    uint8_t hue = isBlue ? 160 : 25; // Blue=160, Orange=25
    fill_solid(leds, NUM_LEDS, CHSV(hue, 255, pulseBrightness));
     FastLED.show();
       lastPulseUpdate = millis();
    }
}
  
bool readButton(){ return digitalRead(BUTTON_PIN)==LOW; }

void sendPressed(){
  struct_message msg;
  msg.fromID = SLAVE_ID;
  strcpy(msg.cmd,"PRESSED");
  esp_now_send(masterMac, (uint8_t*)&msg, sizeof(msg));
  Serial.println("[SLAVE] Sent PRESSED to Master");
}

// ---------------- ESP-NOW Callbacks ----------------
void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len){
  memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));
  Serial.printf("[SLAVE %d] From %d: %s\n", SLAVE_ID, incomingMessage.fromID, incomingMessage.cmd);

// Mark as connected on first message from master
  if (!connectedToMaster) {
    connectedToMaster = true;
    Serial.println("[SLAVE] Connected to Master!");
  }

  if (strcmp(incomingMessage.cmd,"START")==0){
    sequenceRunning=true; pressedYet=false; setAll(CRGB::White);
  }
  else if (strcmp(incomingMessage.cmd,"RESET")==0){
    sequenceRunning=false; pressedYet=false; setAll(CRGB::Blue);
  }
  else if (strcmp(incomingMessage.cmd,"SUCCESS")==0){
    setAll(CRGB::Green);
  }
  else if (strcmp(incomingMessage.cmd,"FAIL")==0){
    setAll(CRGB::Red);
  }
  else if (strcmp(incomingMessage.cmd,"OUT_OF_SEQ")==0){
    Serial.println("[SLAVE] Out-of-sequence! Flashing red.");
    for (int i=0;i<3;i++){ setAll(CRGB::Red); delay(200); setAll(CRGB::Black); delay(200);}
    if (sequenceRunning && !pressedYet) setAll(CRGB::White); // restore state
  }
}

void onDataSent(const uint8_t *mac, esp_now_send_status_t status){
  Serial.print("[SLAVE] Send status: ");
  Serial.println(status==ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ---------------- Setup ----------------
void setup(){
  Serial.begin(115200);
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  FastLED.addLeds<WS2812,LED_PIN,GRB>(leds,NUM_LEDS);
  FastLED.setBrightness(80);
  setAll(CRGB::Blue);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init()!=ESP_OK){ Serial.println("ESP-NOW init failed"); return; }
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, masterMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("[SLAVE] Failed to add Master peer");
  } else {
    Serial.println("[SLAVE] Master peer added.");
  }
}

// ---------------- Loop ----------------
void loop(){
  if (!connectedToMaster) {
//     updateRainbow();
     updateBlueOrangePulse();
     return;
   }
    
  if (sequenceRunning && !pressedYet && readButton()){
    setAll(CRGB::Purple); // valid press = stays purple
    sendPressed();
    pressedYet=true;
    delay(300);
  }
}
