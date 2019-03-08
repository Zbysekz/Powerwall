
#include <ESP8266WiFi.h>

// OLED display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "wifiCredentials.h"

#include <Wire.h>
#include <SPI.h>

const char* ssid = WIFI_SSID;
const char* password = WIFI_PSWD;

// the IP address for the shield:
IPAddress ip(192, 168, 0, 12);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

IPAddress ipServer(192, 168, 0, 3);  

WiFiClient wifiClient;

Adafruit_SSD1306 display;

bool connectedToWifi = false;
bool serverCommState = false;
uint8_t i2cstatus;
int cntRetryConnect=0,cntCommData=0,cntDelayAfterStart=0,cntScanModules=0,cntCalibData=30;
float temp,pressure;
int supplyVoltage;
int i;

#define RXBUFFSIZE 60
#define RXQUEUESIZE 5

uint8_t rxBuffer[RXQUEUESIZE][RXBUFFSIZE];
uint8_t rxBufferLen[RXQUEUESIZE];//>0 if command is inside, 0 is after it is proccessed
uint8_t rxLen,crcH,crcL,readState,rxPtr,rxBufPtr=0;
uint16_t crcReal;



//--------------------------------------------------------------------------------------------------


//If we send a cmdByte with BIT 6 set its a command byte which instructs the cell to do something (not for reading)
#define COMMAND_BIT 6

#define COMMAND_green_led_pattern   1
//unused #define COMMAND_led_off   2
#define COMMAND_factory_default 3
#define COMMAND_set_slave_address 4
#define COMMAND_green_led_default 5
#define COMMAND_set_voltage_calibration 6
#define COMMAND_set_temperature_calibration 7
#define COMMAND_set_bypass_voltage 8
#define COMMAND_set_load_resistance 9


#define read_voltage 10
#define read_temperature 11
#define read_voltage_calibration 12
#define read_temperature_calibration 13
#define read_raw_voltage 14
#define read_error_counter 15
#define read_bypass_enabled_state 16
#define read_bypass_voltage_measurement 17

//Default values
struct cell_module {
  // 7 bit slave I2C address
  uint8_t address; //module address
  uint16_t voltage;//voltage [mV]
  uint16_t temperature;//temperature of NTC sensor [°C]
  
  float voltageCalib;//voltage calibration offset constant
  float temperatureCalib;//temperature calibration offset constant

  bool factoryReset;

  //Record min/max volts over time (between cpu resets)
  uint16_t minVoltage;
  uint16_t maxVoltage;

  bool validValues;

  bool updateCalibration;
};

//Default i2c SLAVE address (used for auto provision of address)
#define DEFAULT_SLAVE_ADDR 21

//Configured cell modules use i2c addresses 24 to 48 (24S)
//See http://www.i2c-bus.org/addressing/
#define MODULE_ADDRESS_RANGE_START 24
#define MODULE_ADDRESS_RANGE_SIZE 24

#define MODULE_ADDRESS_RANGE_END (MODULE_ADDRESS_RANGE_START + MODULE_ADDRESS_RANGE_SIZE)



cell_module moduleList[MODULE_ADDRESS_RANGE_SIZE];
uint8_t modulesCount=0;


union {
  float val;
  uint8_t buffer[4];
} float_to_bytes;

union {
  uint16_t val;
  uint8_t buffer[2];
} uint16_t_to_bytes;



