#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "main.h"

bool led0=false;

// Update these with values suitable for your network.
const char* ssid = "Musquetteer_AP";
const char* password = "RaspberryPi";
const char* mqtt_server = "192.168.42.1";
const String host = String("ESPutnik-") + String(ESP.getChipId(), HEX);

WiFiClient espWifi;
AsyncMqttClient espMQTT;

void setup() {
  Serial.begin(115200);
  delay(100);
  // Set hostname.
  WiFi.hostname(host);
  Serial.println("\n\nHostname: " + host);

  // Configure IO pins
  setup_io();

  // Connect to WiFi network
  setup_wifi();

  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin(host.c_str())) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
  }

  // Set up MQTT server connection
  setup_mqtt();
}

void loop() {
  if (!digitalRead(D5)) {
    led0=!led0;
    if (led0) {
      espMQTT.publish("test/led0", 0, false, "1");
    } else {
      espMQTT.publish("test/led0", 0, false, "0");
    }
    delay(500);
  }
  if (!digitalRead(D6)) {
    espMQTT.publish("test/led0", 0, false, "0");
    espMQTT.publish("test/led1", 0, false, "0");
    espMQTT.publish("test/led2", 0, false, "0");
    espMQTT.publish("test/led3", 0, false, "0");
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
  // Configure WiFi modes
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  delay(50);

  // Start generating own WiFi network
  // Serial.println();
  // Serial.print("Starting AP");
  //
  // WiFi.softAP(host);
  //
  // Serial.printf("WiFi connected [%s]\n", host);
  // Serial.printf("MAC -> %s\n",WiFi.softAPmacAddress().c_str());
  // Serial.print("IP -> ");
  // Serial.println(WiFi.softAPIP());

  // Start connecting to a WiFi network
  Serial.printf("Connecting to WiFi [%s]\n", ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(250);
  }
  Serial.println("\nConnection successful");
  Serial.printf("MAC -> %s\n",WiFi.macAddress().c_str());
  Serial.print("IP -> ");
  Serial.println(WiFi.localIP());
}

void setup_mqtt() {
  espMQTT.onConnect(onMqttConnect);
  espMQTT.onDisconnect(onMqttDisconnect);
  espMQTT.onSubscribe(onMqttSubscribe);
  espMQTT.onUnsubscribe(onMqttUnsubscribe);
  espMQTT.onMessage(onMqttMessage);
  espMQTT.onPublish(onMqttPublish);
  espMQTT.setServer(mqtt_server, 1883);
  espMQTT.setKeepAlive(5).setWill(WILL_TOPIC, WILL_QOS, WILL_RETAIN, WILL_MSG).setClientId(host.c_str());
  Serial.println("Connecting to MQTT...");
  espMQTT.connect();
}

void onMqttConnect() {
  Serial.println("** Connected to the broker **");
  // Once connected, publish status...
  uint16_t packetIdPub2 = espMQTT.publish(WILL_TOPIC, WILL_QOS, WILL_RETAIN, "ESPutnik online");
  Serial.print("Publishing status at QoS 2, packetId: ");
  Serial.println(packetIdPub2);

  // ... and resubscribe to topics
  espMQTT.subscribe("test/led0",0);
  espMQTT.subscribe("test/led1",0);
  espMQTT.subscribe("test/led2",0);
  espMQTT.subscribe("test/led3",0);
  espMQTT.subscribe("test/reset",0);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("** Disconnected from the broker **");
  Serial.print("  Error code: ");
  Serial.println((int8_t)reason);
  WiFi.waitForConnectResult();
  if (WiFi.status() == WL_CONNECTED){
    Serial.println("Reconnecting to MQTT...");
    espMQTT.connect();
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("** Subscribe acknowledged **");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("** Unsubscribe acknowledged **");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t length, size_t index, size_t total) {
  Serial.println("** Publish received **");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  payload: ");
  for (int i = 0; i < length; i++) Serial.print(payload[i]);

  Serial.println();

  if (!strcmp(topic,"test/reset")){
      if ((char)payload[0] == '1') {
        espMQTT.publish("test/text", 0, false, "Reseting ESPutnik");
        espMQTT.publish("test/reset", 0, false, "0");
        ESP.reset();
      }
  }else if (!strcmp(topic,"test/led0")){
      if (!strcmp(payload,"on")) {
        digitalWrite(D0, HIGH);
        espMQTT.publish("test/text", 0, false, "Turning led0 ON");
        led0=true;
      } else {
        digitalWrite(D0, LOW);
        espMQTT.publish("test/text", 0, false, "Turning led0 OFF");
        led0=false;
      }
  }else if (!strcmp(topic,"test/led1")){
      if (!strcmp(payload,"1")) {
        digitalWrite(D1, HIGH);
        espMQTT.publish("test/text", 0, false, "Turning led1 ON");
      } else {
        digitalWrite(D1, LOW);
        espMQTT.publish("test/text", 0, false, "Turning led1 OFF");
      }
  }else if (!strcmp(topic,"test/led2")){
      if (payload[0] == '1') {
        digitalWrite(D2, HIGH);
        espMQTT.publish("test/text", 0, false, "Turning led2 ON");
      } else {
        digitalWrite(D2, LOW);
        espMQTT.publish("test/text", 0, false, "Turning led2 OFF");
      }
  }else if (!strcmp(topic,"test/led3")){
      if ((char)payload[0] == '1') {
        digitalWrite(D3, HIGH);
        espMQTT.publish("test/text", 0, false, "Turning led3 ON");
      } else {
        digitalWrite(D3, LOW);
        espMQTT.publish("test/text", 0, false, "Turning led3 OFF");
      }
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("** Publish acknowledged **");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}
