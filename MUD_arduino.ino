#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// WiFi 
const char* ssid = "";
const char* password = "";

// Your GCP Info
const char* serverIP = "34.94.91.89";
const int serverPort = 8888;
WiFiClient client;

const char* mqttServer = serverIP;  
const int mqttPort = 1883;
const char* mqttTopic = "MUD/playerMove";
WiFiClient espClient;
PubSubClient mqttClient(espClient);

#define BTN_NORTH 12
#define BTN_SOUTH 13
#define BTN_EAST  14
#define BTN_WEST  15
#define SDA 14
#define SCL 13
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Connection retry
unsigned long lastRetryTime = 0;
bool connected = false;

void setup() {
  Serial.begin(115200);

  pinMode(BTN_NORTH, INPUT_PULLUP);
  pinMode(BTN_SOUTH, INPUT_PULLUP);
  pinMode(BTN_EAST, INPUT_PULLUP);
  pinMode(BTN_WEST, INPUT_PULLUP);

  Wire.begin(SDA, SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Welcome to the");
  lcd.setCursor(0, 1);
  lcd.print("Game");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected.");

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
}

//
void tcpReconnect() {
  if (millis() - lastRetryTime >= 2000) {
    Serial.println("Attempting to connect to TCP");
    connected = client.connect(serverIP, serverPort);
    if (connected) {
      Serial.println("Connected to TCP");
    } else {
      Serial.println("Failed to connect to TCP");
    }
    lastRetryTime = millis();
  }
}

// MQTT reconnect
void mqttReconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting to connect to MQTT");
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("Connected to MQTT");
      mqttClient.subscribe(mqttTopic);
    } else {
      Serial.print("Failed to connect to MQTT");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

// Receives messages from MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char msg[257];
  length = min(length, sizeof(msg) - 1);
  memcpy(msg, payload, length);
  msg[length] = '\0';

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Room:");

  if (length <= 16) {
    lcd.setCursor(0, 1);
    lcd.print(msg);
  } else {
    for (int i = 0; i <= length - 16; i++) {
      lcd.setCursor(0, 1);
      lcd.print("                "); 
      lcd.setCursor(0, 1);
      for (int j = 0; j < 16; j++) {
        lcd.print(msg[i + j]);
      }
      delay(300);
    }
  }
}

// Send moves to GCP through TCP connection
void sendMove(int pin, const char* label, const char* msg) {
  if (digitalRead(pin) == LOW) {
    Serial.println(label);
    if (connected) {
      client.println(msg);
    } else {
      Serial.println("Not connected to TCP");
    }
    delay(300);
  }
}

// Retry in case program stops
void loop() {
  if (!connected || !client.connected()) {
    connected = false;
    client.stop();
    tcpReconnect();
  }

  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();

  sendMove(BTN_NORTH, "NORTH", "north");
  sendMove(BTN_SOUTH, "SOUTH", "south");
  sendMove(BTN_EAST,  "EAST",  "east");
  sendMove(BTN_WEST,  "WEST",  "west");
}