#include <Arduino.h>
#include "main.h"

bool led0=false;

//WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
Esp8266Configuration configuration;
AsyncMqttClient mqttClient;

void setup() {
  Serial.begin(115200);
  delay(100);
  // Set hostname.
  WiFi.hostname(HOSTNAME);
  Serial.printf("\n\n\rHostname: %s\n\r",HOSTNAME);

  // Load configuration
  load_config();

  // Configure IO pins
  setup_io();

  // Set up MQTT server connection
  setup_mqtt();

  // Set up OTA updates
  setup_OTA();

  // Connect to WiFi network
  setup_wifi();
}

void loop() {
  ArduinoOTA.handle();
  yield();
  if (!digitalRead(D5)) {
    while (!digitalRead(D5));
    led0=!led0;
    if (led0) {
      mqttClient.publish("test/led0", 0, false, "on");
    } else {
      mqttClient.publish("test/led0", 0, false, "off");
    }
  }
  if (!digitalRead(D6)) {
    while (!digitalRead(D6));
    mqttClient.publish("test/led0", 0, false, "off");
    mqttClient.publish("test/led1", 0, false, "off");
    mqttClient.publish("test/led2", 0, false, "off");
    mqttClient.publish("test/led3", 0, false, "off");
  }
}

void load_config() {
  // read configuration from spiffs
  Serial.println("** Loading configuration from SPIFFS **");
  configuration.read();

  if (configuration.isMqttConfigurationValid() && configuration.isWifiStationConfigurationValid() && configuration.isWifiApConfigurationValid()) {
    Serial.println("Configuration file loaded successfully.");
  }else{
    Serial.println("Configuration file error, defaults loaded.");
    // write default configuration
    configuration.setWifiStationSsid((char*)DEFAULT_STA_SSID);
    configuration.setWifiStationPassword((char*)DEFAULT_STA_PASSWORD);
    configuration.setWifiApSsid((char*)DEFAULT_AP_SSID);
    configuration.setWifiApPassword((char*)DEFAULT_AP_PASSWORD);
    configuration.setMqttServer((char*)DEFAULT_MQTT_SERVER);
    configuration.setMqttPort(atoi(DEFAULT_MQTT_PORT));
    configuration.setMqttDeviceName((char*)HOSTNAME);
    configuration.write();
    configuration.read();
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
  delay(50);

  // define which wifi mode should be used. May be added as helper function to configuration in future
  bool enableAp = false;
  bool enableStation = false;
  if (configuration.isWifiStationEnabled() && configuration.isWifiStationConfigurationValid()) {
    enableStation = true;
  } else {
    enableAp = true;
  }
  if (configuration.isWifiApEnabled()) {
    enableAp = true;
  }
  if (enableAp) {
    if (enableStation) {
      WiFi.mode(WIFI_AP_STA);
    } else {
      WiFi.mode(WIFI_AP);
    }
  } else if (enableStation) {
    WiFi.mode(WIFI_STA);
  }

  if (enableAp) {
    // Start generating own WiFi network
    Serial.println();
    Serial.println("** Starting WiFi hotspot **");

    if(WiFi.softAP(configuration.getWifiApSsid(),configuration.getWifiApPassword())){
      Serial.printf("  SSID: %s\n\r", configuration.getWifiApSsid());
      Serial.printf("  Password: %s\n\r",configuration.getWifiApPassword());
      Serial.printf("  MAC: %s\n\r",WiFi.softAPmacAddress().c_str());
      Serial.printf("  IP: %s\n\r", WiFi.softAPIP().toString().c_str());
    }
  }

  if (enableStation) {
    // Start connecting to a WiFi network
    Serial.println("** Connecting to WiFi **");
    Serial.printf("  SSID: %s\n\r", configuration.getWifiStationSsid());
    Serial.printf("  Password: %s\n\r",configuration.getWifiStationPassword());
    WiFi.begin(configuration.getWifiStationSsid(),configuration.getWifiStationPassword());
  }

  switch (WiFi.waitForConnectResult()) {
//     case WL_CONNECTED:
//       Serial.println("  Connection established");
//       break;
    case WL_IDLE_STATUS:
      Serial.println("  Wi-Fi is in process of changing between statuses");
      break;
    case WL_NO_SSID_AVAIL:
      Serial.println("  SSID cannot be reached");
      break;
    case WL_CONNECT_FAILED:
      Serial.println("  Incorrect password");
      break;
    case WL_CONNECTION_LOST:
      Serial.println("  Connection lost");
      break;
    case WL_DISCONNECTED:
      Serial.println("  Module is not configured in station mode");
      break;
  }
}

void setup_OTA() {
  ArduinoOTA.onStart(onOTAStart);
  ArduinoOTA.onEnd(onOTAEnd);
  ArduinoOTA.onProgress(onOTAProgress);
  ArduinoOTA.onError(onOTAError);
  MDNS.addService("OTA", "udp", 54039);
}

void setup_mqtt() {
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(configuration.getMqttServer(), configuration.getMqttPort());
  mqttClient.setKeepAlive(15).setWill(WILL_TOPIC, WILL_QOS, WILL_RETAIN, WILL_MSG).setClientId(configuration.getMqttDeviceName());
}

WiFiEventHandler connectedEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event) {
  Serial.println("** WiFi connected **");
  Serial.printf("  MAC: %s\n\r",WiFi.macAddress().c_str());
  Serial.printf("  IP: %s\n\r", WiFi.localIP().toString().c_str());

  if (!MDNS.begin(HOSTNAME)) {
    Serial.println("  Error setting up mDNS responder!");
  } else {
    Serial.println("  mDNS responder started");
  }
  Serial.println("  Starting OTA service...");
  ArduinoOTA.begin();
  Serial.println("  Connecting to MQTT...");
  mqttClient.connect();
});

WiFiEventHandler disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
  Serial.println("** Lost WiFi connection **");
});

void onMqttConnect() {
  Serial.println("** Connected to the broker **");
  // Once connected, publish status...
  uint16_t packetIdPub2 = mqttClient.publish(WILL_TOPIC, WILL_QOS, WILL_RETAIN, "ESPutnik online");
  Serial.print("  Publishing status at QoS 2, packetId: ");
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
  switch ((int8_t)reason) {
    case 0:
      Serial.println("TCP disconnected");
      break;
    case 1:
      Serial.println("Connection Refused, unacceptable protocol version");
      break;
    case 2:
      Serial.println("Connection Refused, identifier rejected");
      break;
    case 3:
      Serial.println("Connection Refused, Server unavailable");
      break;
    case 4:
      Serial.println("Connection Refused, bad user name or password");
      break;
    case 5:
      Serial.println("Connection Refused, not authorized");
      break;
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
    if (!strcmp(payload,"on")) {
      mqttClient.publish("test/text", 0, false, "Reseting ESPutnik");
      mqttClient.publish("test/reset", 0, false, "off");
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
