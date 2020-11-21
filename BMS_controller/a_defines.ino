

#include <U8x8lib.h> //OLED display https://github.com/olikraus/u8g2 (U8G2 in library manager)

#include <Ethernet.h>//for ethernet shield
#include <SPI.h>
#include <I2C.h>
#include <avr/wdt.h>


// the IP address for the shield:
IPAddress ip(192, 168, 0, 12);
IPAddress ipServer(192, 168, 0, 3); 

const byte mac[] = {0xDD, 0xAA, 0xBE, 0xEF, 0xFE, 0xED};

EthernetClient ethClient;

U8X8_SSD1306_128X64_NONAME_HW_I2C display; //U8X8_SSD1306_128X64_NONAME_HW_I2C


bool xFullReadDone;
//timers
unsigned long tmrStartTime,tmrServerComm,tmrScanModules,tmrDisplay,tmrPageChange,tmrRetryScan;
//commands
bool xCalibDataRequested;
//statuses
uint8_t status_i2c, status_eth;
uint8_t errorCnt_dataCorrupt, errorCnt_CRCmismatch, errorCnt_BufferFull;

uint8_t showPage;//what page is shown on the display

#define RXBUFFSIZE 20
#define RXQUEUESIZE 3

uint8_t rxBuffer[RXQUEUESIZE][RXBUFFSIZE];//for first item if >0 command is inside, 0 after it is proccessed
bool rxBufferMsgReady[RXQUEUESIZE];
uint8_t rxLen,crcH,crcL,readState,rxPtr,rxBufPtr=0;
uint16_t crcReal;
bool displayOk;

//---------------------- SYSTEM PARAMETERS ---------------------------------------------------------
#define REQUIRED_CNT_MODULES 6

#define REQUIRED_RACK_TEMPERATURE 270 //0,1°C

//discharging voltage range
#define DISCHARGE_VOLT_LOW 330 //x10mV
#define DISCHARGE_VOLT_HIGH 420 //x10mV
//charge voltage range
#define CHARGE_VOLT_LOW 290 //xmV
#define CHARGE_VOLT_HIGH 410 //xmV

//---------------------- PIN DEFINITIONS -----------------------------------------------------------

#define PIN_MAIN_RELAY 5
#define PIN_HEATING 2

#define PIN_UPS_BTN 7

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
  uint16_t voltage;//voltage [10mV]
  uint16_t temperature;//temperature of PTC sensor [0,1°C]
  
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

//Configured cell modules use i2c addresses 24 to 36 (12S)
//See http://www.i2c-bus.org/addressing/
#define MODULE_ADDRESS_RANGE_START 24
#define MODULE_ADDRESS_RANGE_SIZE 12

#define MODULE_ADDRESS_RANGE_END (MODULE_ADDRESS_RANGE_START + MODULE_ADDRESS_RANGE_SIZE)

cell_module moduleList[MODULE_ADDRESS_RANGE_SIZE];
uint8_t modulesCount=0;

//HEATING
bool xHeating;

//StateMachine
uint8_t stateMachineStatus,iBtnStartCnt;
unsigned long tmrDelay;
bool xConnectBattery,xDisconnectBattery,xResetRequested;

union {
  float val;
  uint8_t buffer[4];
} float_to_bytes;

union {
  uint16_t val;
  uint8_t buffer[2];
} uint16_t_to_bytes;
