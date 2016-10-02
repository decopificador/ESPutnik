#ifndef MAIN_H_
#define MAIN_H_

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <AsyncMqttClient.h>
#include <Configuration.h>
#include <FS.h>
#include <ESP8266WebServer.h>
#include <ESP8266FtpServer.h>

#define D0    16  // Wake from sleep
#define D1    5   // I2C Bus SCL (clock)
#define D2    4   // I2C Bus SDA (data)
#define D3    0   // Flash button
#define D4    2   // TX1 (Serial transmit only)
#define D5    14  // SPI Bus SCK (clock)
#define D6    12  // SPI Bus MISO
#define D7    13  // SPI Bus MOSI
#define D8    15  // SPI Bus SS (CS)
#define D9    3   // RX0 (Serial console)
#define D10   1   // TX0 (Serial console)
#define SDCLK 6   // SDIO clock
#define SDD0  7   // SDIO data0
#define SDD1  8   // SDIO data1
#define SDD2  9   // SDIO data2
#define SDD3  10  // SDIO data3
#define SDCMD 11  // SDIO command

#define WILL_TOPIC "test/will"
#define WILL_QOS 1
#define WILL_RETAIN true
#define WILL_MSG "Connection lost!"

const String host = "ESPutnik-" + String(ESP.getChipId(), HEX);
#define HOSTNAME host.c_str()
#define DEFAULT_STA_SSID "Musquetteer_AP"
#define DEFAULT_STA_PASSWORD "RaspberryPi"
#define DEFAULT_AP_SSID HOSTNAME
#define DEFAULT_AP_PASSWORD "ESPutnik"
#define DEFAULT_MQTT_SERVER "192.168.42.1"
#define DEFAULT_MQTT_PORT "1883"
#define DEFAULT_MQTT_CLIENT_ID HOSTNAME

void load_config(void);
void setup_io(void);
void setup_wifi(void);
void setup_OTA(void);
void setup_mqtt(void);
void setup_httpserver(void);
void onHttpGetRoot(void);
void onHttpGetUpdate(void);
void onHttpPostUpdate(void);
void onHttpFileUpload(void);
void onHttpNotFound(void);
void onMqttConnect(void);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttSubscribe(uint16_t packetId, uint8_t qos);
void onMqttUnsubscribe(uint16_t packetId);
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
void onMqttPublish(uint16_t packetId);
void onMqttPublish(uint16_t packetId);
void onOTAStart(void);
void onOTAEnd(void);
void onOTAProgress(unsigned int progress, unsigned int total);
void onOTAError(ota_error_t error);
void onWifiStationConnected(const WiFiEventStationModeGotIP& event);
void onWifiStationDisconnected(const WiFiEventStationModeDisconnected& event);

#endif  // MAIN_H_
