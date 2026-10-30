#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

// ---------- Settings ----------
const char* DEVICE_ID = "led1";
const int   LED_PIN   = 2;        // 2 = board ki built-in LED, 4 = external LED

// Topics (group names)
String T_CMD    = String("home/") + DEVICE_ID + "/cmd";
String T_STATE  = String("home/") + DEVICE_ID + "/state";
String T_STATUS = String("home/") + DEVICE_ID + "/status";

WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);
unsigned long lastReconnectAttempt = 0;

// ---------- LED on/off karo aur broker ko batao ----------
void applyState(const String& s) {
  bool on = (s == "on");
  digitalWrite(LED_PIN, on ? HIGH : LOW);
  mqtt.publish(T_STATE.c_str(), on ? "on" : "off", true);  // true = broker yaad rakhe
  Serial.println("LED -> " + String(on ? "on" : "off"));
}

// ---------- Jab koi message aaye (jaise socket.on('message')) ----------
void onMessage(char* topic, byte* payload, unsigned int len) {
  String msg;
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
  msg.trim();
  Serial.printf("Message [%s]: %s\n", topic, msg.c_str());
  if (String(topic) == T_CMD && (msg == "on" || msg == "off")) applyState(msg);
}

// ---------- WiFi connect ----------
void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("WiFi connecting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(250); Serial.print(".");
    if (millis() - start > 20000) { Serial.println("\nWiFi timeout, restart"); ESP.restart(); }
  }
  Serial.println("\nWiFi OK, IP: " + WiFi.localIP().toString());
}

// ---------- Broker se connect ----------
bool connectMQTT() {
  String clientId = String("esp32-") + DEVICE_ID;
  // Last Will: agar ESP32 gayab ho jaye to broker khud "offline" bhej de
  bool ok = mqtt.connect(clientId.c_str(), nullptr, nullptr,
                         T_STATUS.c_str(), 1, true, "offline");
  if (ok) {
    Serial.println("MQTT connected");
    mqtt.publish(T_STATUS.c_str(), "online", true);
    mqtt.subscribe(T_CMD.c_str(), 1);                       // cmd topic sunna shuru
    mqtt.publish(T_STATE.c_str(), digitalRead(LED_PIN) ? "on" : "off", true);
  } else {
    Serial.printf("MQTT failed, rc=%d\n", mqtt.state());
  }
  return ok;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  ensureWiFi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setKeepAlive(15);
}

void loop() {
  ensureWiFi();
  if (!mqtt.connected()) {
    if (millis() - lastReconnectAttempt > 3000) {   // har 3 sec retry, bina rukne ke
      lastReconnectAttempt = millis();
      connectMQTT();
    }
  } else {
    mqtt.loop();   // messages receive karta hai — zaroori hai
  }
}
