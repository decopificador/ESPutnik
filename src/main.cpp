#include <Arduino.h>
#include "main.h"

bool led0=false;

// Update these with values suitable for your network.
const char* ssid = "Musquetteer_AP";
const char* password = "RaspberryPi";
const char* mqtt_server = "192.168.42.1";
const String host = String("ESPutnik-") + String(ESP.getChipId(), HEX);

AsyncMqttClient mqttClient;

void setup() {
  Serial.begin(115200);
  delay(100);
  // Set hostname.
  WiFi.hostname(host);
  Serial.println("\n\n\rHostname: " + host);

  // Configure IO pins
  setup_io();

  // Set up MQTT server connection
  setup_mqtt();

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

  // Set up OTA updates
  setup_OTA();
}

void loop() {
  ArduinoOTA.handle();
  yield();
  if (!digitalRead(D5)) {
    led0=!led0;
    if (led0) {
      mqttClient.publish("test/led0", 0, false, "1");
    } else {
      mqttClient.publish("test/led0", 0, false, "0");
    }
    delay(500);
  }
  if (!digitalRead(D6)) {
    mqttClient.publish("test/led0", 0, false, "0");
    mqttClient.publish("test/led1", 0, false, "0");
    mqttClient.publish("test/led2", 0, false, "0");
    mqttClient.publish("test/led3", 0, false, "0");
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
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(onSTAConnected, WIFI_EVENT_STAMODE_GOT_IP);
  WiFi.onEvent(onSTADisconnected, WIFI_EVENT_STAMODE_DISCONNECTED);
//  WiFi.onEvent(onAPConnected, WIFI_EVENT_SOFTAPMODE_STACONNECTED);
//  WiFi.onEvent(onAPDisconnected, WIFI_EVENT_SOFTAPMODE_STADISCONNECTED);
  delay(50);

  WiFiMode_t radio = WiFi.getMode();
  if ((radio == WIFI_AP) || (radio == WIFI_AP_STA )) {
    // Start generating own WiFi network
    Serial.println();
    Serial.println("Starting WiFi hotspot");

    if(WiFi.softAP(host.c_str())){
      Serial.printf("Hotspot deployed [%s]\n\r", host.c_str());
      Serial.printf("MAC -> %s\n\r",WiFi.softAPmacAddress().c_str());
      Serial.print("IP -> ");
      Serial.println(WiFi.softAPIP());
    }
  }

  if ((radio == WIFI_STA) || (radio == WIFI_AP_STA )) {
    // Start connecting to a WiFi network
    Serial.printf("Connecting to WiFi [%s]\n\r", ssid);
    WiFi.begin(ssid, password);
  }

  switch (WiFi.waitForConnectResult()) {
    // case WL_CONNECTED:
    //   Serial.println("Connection established");
    //   break;
    case WL_IDLE_STATUS:
      Serial.println("Wi-Fi is in process of changing between statuses");
      break;
    case WL_NO_SSID_AVAIL:
      Serial.println("SSID cannot be reached");
      break;
    case WL_CONNECT_FAILED:
      Serial.println("Incorrect password");
      break;
    case WL_CONNECTION_LOST:
      Serial.println("Connection lost");
      break;
    case WL_DISCONNECTED:
      Serial.println("Module is not configured in station mode");
      break;
  }
}

void setup_OTA() {
  ArduinoOTA.begin();
  ArduinoOTA.onStart(onOTAStart);
  ArduinoOTA.onEnd(onOTAEnd);
  ArduinoOTA.onProgress(onOTAProgress);
  ArduinoOTA.onError(onOTAError);
}

void setup_mqtt() {
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setKeepAlive(15).setWill(WILL_TOPIC, WILL_QOS, WILL_RETAIN, WILL_MSG).setClientId(host.c_str());
}

void onSTAConnected(WiFiEvent_t event) {
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onSTADisconnected(WiFiEvent_t event) {
  Serial.println("Lost WiFi connection");
}

void onMqttConnect() {
  Serial.println("** Connected to the broker **");
  // Once connected, publish status...
  uint16_t packetIdPub2 = mqttClient.publish(WILL_TOPIC, WILL_QOS, WILL_RETAIN, "ESPutnik online");
  Serial.print("Publishing status at QoS 2, packetId: ");
  Serial.println(packetIdPub2);

  // ... and resubscribe to topics
  mqttClient.subscribe("test/led0",0);
  mqttClient.subscribe("test/led1",0);
  mqttClient.subscribe("test/led2",0);
  mqttClient.subscribe("test/led3",0);
  mqttClient.subscribe("test/reset",0);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("** Disconnected from the broker **");
  Serial.print("  Error code: ");
  Serial.println((int8_t)reason);
  if (WiFi.waitForConnectResult() == WL_CONNECTED){
    Serial.println("Reconnecting to MQTT...");
    mqttClient.connect();
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
  Serial.println(payload);

  if (!strcmp(topic,"test/reset")){
    if ((char)payload[0] == '1') {
      mqttClient.publish("test/text", 0, false, "Reseting ESPutnik");
      mqttClient.publish("test/reset", 0, false, "0");
      ESP.reset();
    }
  }else if (!strcmp(topic,"test/led0")){
    if (!strcmp(payload,"on")) {
      digitalWrite(D0, HIGH);
      mqttClient.publish("test/text", 0, false, "Turning led0 ON");
      led0=true;
    } else {
      digitalWrite(D0, LOW);
      mqttClient.publish("test/text", 0, false, "Turning led0 OFF");
      led0=false;
    }
  }else if (!strcmp(topic,"test/led1")){
    if (!strcmp(payload,"on")) {
      digitalWrite(D1, HIGH);
      mqttClient.publish("test/text", 0, false, "Turning led1 ON");
    } else {
      digitalWrite(D1, LOW);
      mqttClient.publish("test/text", 0, false, "Turning led1 OFF");
    }
  }else if (!strcmp(topic,"test/led2")){
    if (!strcmp(payload,"on")) {
      digitalWrite(D2, HIGH);
      mqttClient.publish("test/text", 0, false, "Turning led2 ON");
    } else {
      digitalWrite(D2, LOW);
      mqttClient.publish("test/text", 0, false, "Turning led2 OFF");
    }
  }else if (!strcmp(topic,"test/led3")){
    if (!strcmp(payload,"on")) {
      digitalWrite(D3, HIGH);
      mqttClient.publish("test/text", 0, false, "Turning led3 ON");
    } else {
      digitalWrite(D3, LOW);
      mqttClient.publish("test/text", 0, false, "Turning led3 OFF");
    }
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("** Publish acknowledged **");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onOTAStart() {
  // String type;
  // if (ArduinoOTA.getCommand() == U_FLASH)
  //   type = "sketch";
  // else // U_SPIFFS
  //   type = "filesystem";
  //
  // // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
  Serial.println("Start updating");
}

void onOTAEnd() {
  Serial.println("\nEnd");
}

void onOTAProgress(unsigned int progress, unsigned int total) {
  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}

void onOTAError(ota_error_t error) {
  Serial.printf("Error[%u]: ", error);
  if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
  else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
  else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
  else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
  else if (error == OTA_END_ERROR) Serial.println("End Failed");
}
