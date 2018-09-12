#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include "wifiCredentials.h"
#include "e_main.h"

#include <Wire.h>
#include <SPI.h>


const char* ssid = WIFI_SSID;
const char* password = WIFI_PSWD;

// the IP address for the shield:
IPAddress ip(192, 168, 0, 10);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

IPAddress ipServer(192, 168, 0, 3);  

WiFiClient wifiClient;

os_timer_t timer1;

bool connectedToWifi = false;
int cntRetryConnect=0,cntSendData=0;
float temp,pressure;
int supplyVoltage;
int i;
bool OTAmode=0;

#define RXBUFFSIZE 60
#define RXQUEUESIZE 5

//variables for communication on WIFI
uint8_t rxBuffer[RXQUEUESIZE][RXBUFFSIZE];
uint8_t rxBufferLen[RXQUEUESIZE];//>0 if command is inside, set 0 when command is processed
uint8_t rxLen,crcH,crcL,readState,rxPtr,rxBufPtr=0;
uint16_t crcReal;
////////

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
#define read_load_resistance 18


//Default values
struct cell_module {
  uint8_t address;        //7 bit slave I2C address
  uint16_t voltage;       //voltage of battery cell
  uint16_t temperature;   //temperature from sensor

  uint16_t bypassVoltage;//voltage when bypass resistor is switched on
  
  float voltage_calib;    //voltage calibration
  float temperature_calib;//temperature calibration
  float loadResistance;   //load resistance

  bool factoryReset;      //if factory reset is requested

  uint16_t min_voltage;   //Record min/max volts over time (not persistent)
  uint16_t max_voltage;

  bool valid_values;      //if values are in valid range

  bool update_calibration;//?
};

//Allow up to 24 modules
cell_module cellArray[24];

uint8_t cellArray_index,cellArray_cnt;
bool runProvisioning=false;

//-----------------------------------I2C----------------------------------------

uint8_t i2cstatus;

//Default i2c SLAVE address (used for auto provision of address)
#define DEFAULT_SLAVE_ADDR 21

//Configured cell modules use i2c addresses 24 to 48 (24S)
//See http://www.i2c-bus.org/addressing/
#define DEFAULT_SLAVE_ADDR_START_RANGE 24
#define DEFAULT_SLAVE_ADDR_END_RANGE DEFAULT_SLAVE_ADDR_START_RANGE + 24

union {
  float val;
  uint8_t buffer[4];
} float_to_bytes;

union {
  uint16_t val;
  uint8_t buffer[2];
} uint16_t_to_bytes;
