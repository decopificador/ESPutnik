#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPUpdateServer.h>
#include <PubSubClient.h>

#define D0 16
#define D1 5 // I2C Bus SCL (clock)
#define D2 4 // I2C Bus SDA (data)
#define D3 0
#define D4 2 // Same as "LED_BUILTIN", but inverted logic
#define D5 14 // SPI Bus SCK (clock)
#define D6 12 // SPI Bus MISO
#define D7 13 // SPI Bus MOSI
#define D8 15 // SPI Bus SS (CS)
#define D9 3 // RX0 (Serial console)
#define D10 1 // TX0 (Serial console)

#define WILL_TOPIC "test/will"
#define WILL_QOS 1
#define WILL_RETAIN true
#define WILL_MSG "Connection lost!"

bool led0=false;

// Update these with values suitable for your network.
const char* ssid = "Musquetteer_AP";
const char* password = "RaspberryPi";
const char* mqtt_server = "192.168.42.1";
const String host = String("ESPutnik-") + String(ESP.getChipId(), HEX);

WiFiClient espWifi;
PubSubClient espMQTT(espWifi);
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;


ADC_MODE(ADC_VCC);
//float servolt1 = ESP.getVcc();

void setup_io();
void setup_wifi();
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);

void setup() {
  Serial.begin(115200);
  delay(1000);
  // Set hostname.
  WiFi.hostname(host);

  Serial.println();
  Serial.println("Hostname: " + host);

  setup_io();
  setup_wifi();
  MDNS.begin(host.c_str());

  espMQTT.setServer(mqtt_server, 1883);
  espMQTT.setCallback(callback);

  httpUpdater.setup(&httpServer);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.println("HTTPUpdateServer ready! Open http://"+host+".local/update in your browser");
}

void loop() {

  if (!espMQTT.connected()) {
    reconnect();
  }

  espMQTT.loop();
  httpServer.handleClient();

  if (!digitalRead(D5)) {
    led0=!led0;
    if (led0) {
      espMQTT.publish("test/led0", "1");
    } else {
      espMQTT.publish("test/led0", "0");
    }
    delay(500);
  }
  if (!digitalRead(D6)) {
    espMQTT.publish("test/led0", "0");
    espMQTT.publish("test/led1", "0");
    espMQTT.publish("test/led2", "0");
    espMQTT.publish("test/led3", "0");
    delay(500);
  }
}

void setup_io(){
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D5, INPUT);
  pinMode(D6, INPUT);
  delay(50);
}

void setup_wifi() {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  delay(10);

  //
  // // Start generating own WiFi network
  // Serial.println();
  // Serial.print("Starting AP");
  //
  // WiFi.softAP(host);
  //
  // Serial.printf("WiFi connected [%s]\n", host);
  // Serial.printf("MAC -> %s\n",WiFi.softAPmacAddress().c_str());
  // Serial.print("IP -> ");
  // Serial.println(WiFi.softAPIP());
  //
  // Start connecting to a WiFi network
  Serial.println();
  Serial.printf("Connecting to WiFi [%s]\n", ssid);

  WiFi.begin(ssid, password);
  WiFi.waitForConnectResult();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connection successful");
    Serial.printf("MAC -> %s\n",WiFi.macAddress().c_str());
    Serial.print("IP -> ");
    Serial.println(WiFi.localIP());
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!espMQTT.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (espMQTT.connect(host.c_str(),WILL_TOPIC,WILL_QOS,WILL_RETAIN,WILL_MSG)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      espMQTT.publish("test/will", "ESPutnik online", true);
      // ... and resubscribe
      espMQTT.subscribe("test/led0");
      espMQTT.subscribe("test/led1");
      espMQTT.subscribe("test/led2");
      espMQTT.subscribe("test/led3");
      espMQTT.subscribe("test/reset");

    } else {
      Serial.print("failed, rc=");
      Serial.print(espMQTT.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (!strcmp(topic,"test/reset")){
      if ((char)payload[0] == '1') {
        espMQTT.publish("test/text","Reseting ESPutnik");
        espMQTT.publish("test/reset","0");
        delay(500);
        ESP.restart();
      }
  }else if (!strcmp(topic,"test/led0")){
      if ((char)payload[0] == '1') {
        digitalWrite(D0, HIGH);   // Turn the LED on (Note that LOW is the voltage level
        // but actually the LED is on; this is because
        // it is acive low on the ESP-01)
        espMQTT.publish("test/text","Turning led0 ON");
        led0=true;
      } else {
        digitalWrite(D0, LOW);  // Turn the LED off by making the voltage HIGH
        espMQTT.publish("test/text","Turning led0 OFF");
        led0=false;
      }
  }else if (!strcmp(topic,"test/led1")){
      if ((char)payload[0] == '1') {
        digitalWrite(D1, HIGH);   // Turn the LED on (Note that LOW is the voltage level
        // but actually the LED is on; this is because
        // it is acive low on the ESP-01)
        espMQTT.publish("test/text","Turning led1 ON");
      } else {
        digitalWrite(D1, LOW);  // Turn the LED off by making the voltage HIGH
        espMQTT.publish("test/text","Turning led1 OFF");
      }
  }else if (!strcmp(topic,"test/led2")){
      if ((char)payload[0] == '1') {
        digitalWrite(D2, HIGH);   // Turn the LED on (Note that LOW is the voltage level
        // but actually the LED is on; this is because
        // it is acive low on the ESP-01)
        espMQTT.publish("test/text","Turning led2 ON");
      } else {
        digitalWrite(D2, LOW);  // Turn the LED off by making the voltage HIGH
        espMQTT.publish("test/text","Turning led2 OFF");
      }
  }else if (!strcmp(topic,"test/led3")){
      if ((char)payload[0] == '1') {
        digitalWrite(D3, HIGH);   // Turn the LED on (Note that LOW is the voltage level
        // but actually the LED is on; this is because
        // it is acive low on the ESP-01)
        espMQTT.publish("test/text","Turning led3 ON");
      } else {
        digitalWrite(D3, LOW);  // Turn the LED off by making the voltage HIGH
        espMQTT.publish("test/text","Turning led3 OFF");
      }
  }
}
