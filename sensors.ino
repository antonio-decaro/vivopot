#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>

#include <PubSubClient.h>
#include <WiFiNINA.h>
#include <dht.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET 4
#define SCREEN_ADDRESS 0x3C

#define LOOP_DELAY 500
#define SEND_DELAY 1000
#define MAX_WIFI_CONNECTING_TIME 10000

#define DHT22_PIN 8
#define LIGHT_SENSOR A3
#define WATER_SENSOR_DO 2
#define WATER_SENSOR_AO A4
#define ANALOG_MQ2_PIN A5

/* Constants */
const char wifi_ssid[] = "WiFi-LabIoT"; 
const char wifi_pass[] = "s1jzsjkw5b";

const char mqtt_broker[] = "192.168.1.119";
const int mqtt_port = 1883;

const char airquality_sensor_topic[]  = "sensors/airquality/out";
const char humidity_sensor_topic[]    = "sensors/humidity/out";
const char light_sensor_topic[]       = "sensors/light/out";
const char temperature_sensor_topic[] = "sensors/temperature/out";
const char water_sensor_topic[]       = "sensors/water/out";

/* Data Definition */
typedef struct sensor_data_struct {
  float air_quality;
  float humidity;
  float light;
  float rain;
  float temperature;
  float water;
} SensorData_t;

/* Global variables */

dht DHT;
SensorData_t sensorData;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
char buff[64];
bool wifiConnected;

/* Functions */

void collect_sensor_data() {
  DHT.read22(DHT22_PIN);
  sensorData.temperature = DHT.temperature;
  sensorData.humidity = DHT.humidity;

  sensorData.air_quality = analogRead(ANALOG_MQ2_PIN);
  sensorData.water = map(analogRead(WATER_SENSOR_AO), 1100, 0, 0, 100);
  sensorData.light = map(analogRead(LIGHT_SENSOR), 0, 1000, 0, 10);
}

void process_sensor_data() {
  /* Print on Serial and on LCD*/

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.cp437(true);
  
  display.setCursor(0, 0);
  display.print(sensorData.temperature);
  display.print(" *C");
  
  display.setCursor(80, 0);
  display.print("H:");
  display.print(sensorData.humidity);
  display.print("%");

  display.setCursor(0, 12);
  display.print("AirQual.:");
  display.print((int) sensorData.air_quality);

  display.setCursor(100, 12);
  display.print("L:");
  display.print((int) sensorData.light);

  display.setCursor(0,24);
  display.print("Moisture:");
  display.print((int) sensorData.water);
  display.print('%');

  display.setCursor(100, 24);
  if (mqttClient.connected())
    display.print("MQTT");
  
  display.display();
}

void send_sensor_data() {
  if (!wifiConnected) {
    return;
  }
  /* Send to MQTT Topics */
  dtostrf(sensorData.air_quality, 4, 0, buff);
  mqttClient.publish(airquality_sensor_topic, buff);
  
  dtostrf(sensorData.humidity, 2, 2, buff);
  mqttClient.publish(humidity_sensor_topic, buff);
  
  dtostrf(sensorData.light, 2, 0, buff);
  mqttClient.publish(light_sensor_topic, buff);
  
//  sprintf(buff, "%f", sensorData.rain);
//  mqttClient.publish(rain_sensor_topic, buff);

  dtostrf(sensorData.temperature, 2, 2, buff);
  mqttClient.publish(temperature_sensor_topic, buff);
  
  dtostrf(sensorData.water, 4, 0, buff);
  mqttClient.publish(water_sensor_topic, buff);
}

void reconnect() {
  // Loop until we're reconnected
  Serial.println("Attempting MQTT connection...");
  while (!mqttClient.connected()) {
    Serial.println("Wifi Connected:" + wifiConnected);
    if (mqttClient.connect("WifiRev2")) {
      Serial.println("MQTT Client connected");
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
      delay(MAX_WIFI_CONNECTING_TIME);
    }
  }
}

/* Sketch */

void setup() {
  // initialize serial
  Serial.begin(9600);

  //initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed!");
    while(1);
  }
  Serial.println("Display initialized!");
  display.display();
  display.clearDisplay();
  
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
    if (!mqttClient.connected()) {
      Serial.println("Setup connection");
      reconnect(); 
    }
  }
  
  // select pinmode
  pinMode(DHT22_PIN, INPUT);
  pinMode(WATER_SENSOR_DO, INPUT);

  Serial.println("Start collecting data...");
}

void loop() {
  if (wifiConnected) {
    mqttClient.loop(); 
  }
 
  collect_sensor_data();
  process_sensor_data();
  send_sensor_data();    
  
  delay(LOOP_DELAY);
}
