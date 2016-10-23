#ifndef MAIN_H_
#define MAIN_H_

#include <ArduinoOTA.h>
#include <Homie.h>

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

static const uint8_t D[]   = {D0,D1,D2,D3,D4,D5,D6,D7,D8,D9,D10};

void setup_io(void);
void setup_OTA(void);
void setup_homie(void);
void onOTAStart(void);
void onOTAEnd(void);
void onOTAProgress(unsigned int progress, unsigned int total);
void onOTAError(ota_error_t error);
void onHomieEvent(HomieEvent event);
bool outNodeHandler(const String& property, const HomieRange& range, const String& value);
bool pwnNodeHandler(const String& property, const HomieRange& range, const String& value);

static HomieNode outNode("out", "digital", outNodeHandler);
static HomieNode inNode("in", "digital");
static HomieNode adcNode("adc", "analog");
static HomieNode pwmNode("pwm", "analog", pwnNodeHandler);
static HomieNode i2cNode("i2c", "bus");
static HomieNode owNode("ow", "bus");
static HomieNode lightNode("light", "switch");

#endif  // MAIN_H_
