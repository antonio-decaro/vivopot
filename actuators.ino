#include <SPI.h>
#include <Wire.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>

#define LOOP_DELAY 500
#define MAX_WIFI_CONNECTING_TIME 5000

#define STATUS_LED_PIN 4
#define LIGHT_PIN 2
#define PUMP_PIN 5

/* Constants */
const char wifi_ssid[] = "WiFi-LabIoT";
const char wifi_pass[] = "s1jzsjkw5b";

const char mqtt_broker[] = "192.168.1.119";
const int mqtt_port = 1883;

const char led_actuator_topic[]  = "actuators/led/in";
const char pump_actuator_topic[]  = "actuators/pump/in";

/* Global Variables */

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
bool wifiConnected;

/* Functions */

void switch_light(String message) {
  char buf[8];

  message.toCharArray(buf, message.length() + 1);

  if (!strcmp(buf, "1")) {
    digitalWrite(LIGHT_PIN, HIGH);
  } else {
    digitalWrite(LIGHT_PIN, LOW);
  }
}

void switch_pump(String message) {
  char buf[8];
  message.toCharArray(buf, message.length() + 1);

  for (int i = 0; i < message.length(); i++) {
    if (!isDigit(buf[i])) {
      return;
    }

    int val = atoi(buf);
    if (val >= 1) {
      Serial.println("Activating pump...");
      digitalWrite(PUMP_PIN, LOW);
      delay(3700);
      Serial.println("Deactivating pump...");
      digitalWrite(PUMP_PIN, HIGH);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: "); Serial.println(topic);
  Serial.print(" - Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++) { 
    Serial.print((char)message[i]); 
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (!strcmp(topic, led_actuator_topic)) {
    switch_light(messageTemp);
  } else if (!strcmp(topic, pump_actuator_topic)) {
    switch_pump(messageTemp);
  }
  
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  Serial.println("Attempting MQTT connection...");
  while (!mqttClient.connected()) {
    if (mqttClient.connect("ArduinoMKRWifi1010")) {
      Serial.println("MQTT Client connected");
      // subscribe to topics
      mqttClient.subscribe(led_actuator_topic);
      mqttClient.subscribe(pump_actuator_topic);
      Serial.print("Subscribed to topic: "); Serial.println(led_actuator_topic);
      Serial.print("Subscribed to topic: "); Serial.println(pump_actuator_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
      delay(MAX_WIFI_CONNECTING_TIME);
    }
  }
}

/* Arduino Functions */

void setup() {
    // initialize pins
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, HIGH);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  
  // initialize serial
  Serial.begin(9600);

  // initialize wifi
  wifiConnected = true;
  int startTime = millis();
  Serial.print("Attempting to connect to SSID: "); Serial.println(wifi_ssid);
  while (WiFi.begin(wifi_ssid, wifi_pass) != WL_CONNECTED) {
    if ((millis() - startTime) >= MAX_WIFI_CONNECTING_TIME) {
      wifiConnected = false;
      break;
    }
    Serial.print(".");
    delay(1000);
  }
  if (wifiConnected) {
    Serial.print("You are connected to the network. Local IP: "); Serial.println(WiFi.localIP()); 
  } else {
    Serial.println("Can not connect to the WiFi network.");
  }

  // connect to MQTT only if is connected to wifi
  if (wifiConnected) {
    mqttClient.setServer(mqtt_broker, mqtt_port);
    mqttClient.setCallback(callback);
    if (!mqttClient.connected()) {
      Serial.println("Setup connection");
      reconnect();
    }
  }
}


void loop() {
  if (wifiConnected)
    mqttClient.loop();

  if (mqttClient.connected()) {
    digitalWrite(STATUS_LED_PIN, HIGH);
  } else {
    digitalWrite(STATUS_LED_PIN, LOW);
    reconnect();
    digitalWrite(STATUS_LED_PIN, HIGH);
  }
    
  delay(LOOP_DELAY);
}
