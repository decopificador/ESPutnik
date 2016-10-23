#include <Arduino.h>
#include "main.h"

const int TEMPERATURE_INTERVAL = 30;
unsigned long lastTemperatureSent = 0;
float temperature;
int adc;
bool lastInState[4], inState[4];

void setupHandler() {
  inNode.advertise("0");
  inNode.advertise("1");
  inNode.advertise("2");
  inNode.advertise("3");
  inNode.advertise("4");
  outNode.advertise("5").settable();
  outNode.advertise("6").settable();
  outNode.advertise("7").settable();
  outNode.advertise("8").settable();
  adcNode.advertise("0");
  adcNode.advertise("unit");
  adcNode.advertise("temp");
  adcNode.setProperty("unit").send("°C");
}

void loopHandler() {
  for (uint8_t i = 0; i < 4; i++) {
    inState[i] = digitalRead(D[i]);
    if (lastInState[i] != inState[i]) {
      inNode.setProperty(String(i)).send(inState[i]?"OPEN":"CLOSED");
      lastInState[i] = inState[i];
    }
  }
  if (millis() - lastTemperatureSent >= TEMPERATURE_INTERVAL * 1000UL || lastTemperatureSent == 0) {
    adc = analogRead(A0);
    adcNode.setProperty("0").send(String(adc));
    temperature = adc/9.76;
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
    adcNode.setProperty("temp").send(String(temperature));
    lastTemperatureSent = millis();
  }
}

void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.printf("\n\n\rBooting...\n\r");

  // Configure IO pins
  setup_io();

  // Set up OTA updates
  setup_OTA();
  Serial.println("** Starting OTA service **");
  ArduinoOTA.begin();
  delay(2000);

  // Set up Homie
  setup_homie();
}

void loop() {
  ArduinoOTA.handle();
  Homie.loop();
}

void setup_io(){
  pinMode(D0, INPUT);
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);
  pinMode(D3, INPUT);
  pinMode(D4, OUTPUT);
  digitalWrite(D4, HIGH);
  pinMode(D5, OUTPUT);
  digitalWrite(D5, LOW);
  pinMode(D6, OUTPUT);
  digitalWrite(D6, LOW);
  pinMode(D7, OUTPUT);
  digitalWrite(D7, LOW);
  pinMode(D8, OUTPUT);
  digitalWrite(D8, LOW);
  delay(50);
}

void setup_OTA() {
  ArduinoOTA.onStart(onOTAStart);
  ArduinoOTA.onEnd(onOTAEnd);
  ArduinoOTA.onProgress(onOTAProgress);
  ArduinoOTA.onError(onOTAError);
  MDNS.addService("OTA", "udp", 54039);
}

void setup_homie() {
  Homie_setFirmware("ESPutnik", "1.0.0");
  Homie_setBrand("ESPutnik");
  Homie.setLedPin(D4, HIGH);
  Homie.onEvent(onHomieEvent);
  Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);
  Homie.setup();
}

void onOTAStart() {
  // String type;
  // if (ArduinoOTA.getCommand() == U_FLASH)
  //   type = "sketch";
  // else // U_SPIFFS
  //   type = "filesystem";
  //
  // // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
  SPIFFS.end();
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

void onHomieEvent(HomieEvent event) {
  switch (event.type) {
    case HomieEventType::STANDALONE_MODE:
      Serial.println("Standalone mode started");
      break;
    case HomieEventType::CONFIGURATION_MODE:
      Serial.println("Configuration mode started");
      break;
    case HomieEventType::NORMAL_MODE:
      Serial.println("Normal mode started");
      break;
    case HomieEventType::OTA_STARTED:
      Serial.println("OTA started");
      break;
    case HomieEventType::OTA_FAILED:
      Serial.println("OTA failed");
      break;
    case HomieEventType::OTA_SUCCESSFUL:
      Serial.println("OTA successful");
      break;
    case HomieEventType::ABOUT_TO_RESET:
      Serial.println("About to reset");
      break;
    case HomieEventType::WIFI_CONNECTED:
      Serial.println("Wi-Fi connected");
      break;
    case HomieEventType::WIFI_DISCONNECTED:
      Serial.println("Wi-Fi disconnected");
      break;
    case HomieEventType::MQTT_CONNECTED:
      Serial.println("MQTT connected");
      break;
    case HomieEventType::MQTT_DISCONNECTED:
      Serial.println("MQTT disconnected");
      break;
  }
}

bool outNodeHandler(const String& property, const HomieRange& range, const String& value){
  if (value != "ON" && value != "OFF") return false;
  bool set = (value == "ON");
  uint8_t i = atoi(property.c_str());
  digitalWrite(D[i], set?HIGH:LOW);
  outNode.setProperty(property.c_str()).send(set?"ON":"OFF");
  return true;
}
