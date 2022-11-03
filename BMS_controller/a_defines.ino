
#include <Ethernet.h>//for ethernet shield
#include <SPI.h>
#include <I2C.h>//library from https://github.com/rambo/I2C
#include <avr/wdt.h>
#include <RunningMedian.h> // bob tillard running median


//---------------------- SYSTEM PARAMETERS ---------------------------------------------------------
// the IP address for the shield:
IPAddress ip(192, 168, 0, 12);
IPAddress ipServer(192, 168, 0, 3); 
IPAddress dns(192, 168, 0, 4);
IPAddress gateway(192, 168, 0, 4);
IPAddress subnet(255, 255, 255, 0);

//never change 0xDE part
uint8_t mac[] = {0xDE, 0xAA, 0xBE, 0xEF, 0xFE, 0xED};

#define REQUIRED_CNT_MODULES 12

#define REQUIRED_RACK_TEMPERATURE 100 //0,1°C

//temperature ok limits
#define LIMIT_RACK_TEMPERATURE_LOW 70 //0,1°C
#define LIMIT_RACK_TEMPERATURE_HIGH 500 //0,1°C

//absolute voltage limits
#define LIMIT_VOLT_LOW2 310 //x10mV - absolute limit, goes to error if below
#define LIMIT_VOLT_LOW 330 //x10mV - limit for auto transition from run to charge
#define LIMIT_VOLT_HIGH 410 //x10mV
#define LIMIT_VOLT_HIGH2 415 //x10mV

//balancing thresholds
#define IMBALANCE_THRESHOLD 10//x10mV
#define IMBALANCE_THRESHOLD_CHARGED 2//x10mV - same as above, but when is battery considered as fully charged

#define CHARGED_LEVEL 4920//x10mV voltage when battery is considered as fully charged

#define RXBUFFSIZE 20
#define RXQUEUESIZE 3

//---------------------- ADDRESSING PARAMETERS ---------------------------------------------------------
//Default i2c SLAVE address (used for auto provision of address)
#define DEFAULT_SLAVE_ADDR 21

//Configured cell modules use i2c addresses up to (12S)
//See http://www.i2c-bus.org/addressing/
#define MODULE_ADDRESS_RANGE_START 24
#define MODULE_ADDRESS_RANGE_SIZE 12

#define MODULE_ADDRESS_RANGE_END (MODULE_ADDRESS_RANGE_START + MODULE_ADDRESS_RANGE_SIZE)

//---------------------- PIN DEFINITIONS -----------------------------------------------------------
#define PIN_MAIN_RELAY 5
#define PIN_SOLAR_CONTACTOR 8
#define PIN_OUTPUT_DCAC_BREAKER 9
#define PIN_VENTILATOR 10

//---------------------- COMMAND DEFINES -----------------------------------------------------------

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
#define COMMAND_set_voltage_calibration2 10
#define COMMAND_set_temperature_calibration2 11
#define COMMAND_resetI2c 12

#define read_voltage 10
#define read_temperature 11
#define read_voltage_calibration 12
#define read_temperature_calibration 13
#define read_raw_voltage 14
#define read_error_counter 15
#define read_bypass_enabled_state 16
#define read_bypass_voltage_measurement 17
#define read_burning_counter 18
#define read_voltage_calibration2 19
#define read_temperature_calibration2 20
#define read_CRC 21



#define VOLT_AVG_SAMPLES 6
#define TEMP_AVG_SAMPLES 6

//---------------------- STRUCTURES -----------------------------------------------------------

struct cell_module {
  // 7 bit slave I2C address
  uint8_t address; //module address
  uint16_t voltage;//voltage [10mV]
  uint16_t temperature;//temperature of PTC sensor [0,1°C]

  uint16_t voltage_avg;
  uint16_t voltage_buff[VOLT_AVG_SAMPLES];
  uint8_t voltAvgPtr;

  uint16_t temperature_avg;
  uint16_t temperature_buff[TEMP_AVG_SAMPLES];
  uint8_t tempAvgPtr;
  
  float voltageCalib;//voltage calibration scaling constant
  float voltageCalib2;//voltage calibration offset constant
  float temperatureCalib;//temperature calibration scaling constant
  float temperatureCalib2;//temperature calibration offset constant

  bool factoryReset;

  //Record min/max volts over time (between cpu resets)
  uint16_t minVoltage;
  uint16_t maxVoltage;

  uint8_t readErrCnt;//how much times we had reading error
  bool validValues;//if we have few read errors or the values are not in good range, they are not valid
  bool burning; // if module is burning or not

  uint16_t iStatErrCnt, iBurningCnt;//statistics
};

//---------------------- ERROR STATUS definitions ---------------------------------------------
// for overallErrorStatus
#define ERROR_ETHERNET 0x01
#define ERROR_I2C 0x02
#define ERROR_MODULE_CNT 0x04
#define ERROR_VOLTAGE_RANGES 0x08
#define ERROR_TEMP_RANGES 0x10


//---------------------- VARIABLES -----------------------------------------------------------

EthernetClient ethClient;

uint8_t sendBuff[32];

bool xFullReadDone,xSafetyConditions;
//timers
unsigned long tmrStartTime,tmrServerComm,tmrScanModules,tmrRetryScan,tmrSendData,tmrReadStatistics,tmrCommTimeout,tmrDelayAfterSolarReconnect;
//commands
bool xCalibDataRequested;
//statuses
uint8_t status_i2c, status_eth;
uint8_t errorCnt_dataCorrupt, errorCnt_CRCmismatch, errorCnt_BufferFull;
bool voltagesOk,validValues,temperaturesOk;
uint8_t errorStatus,errorStatus_cause;
bool xServerEndPacket,oneOfCellIsLow, oneOfCellIsHigh, solarConnected;
uint8_t iFailCommCnt;

int errorWhichModule;
uint8_t rxBuffer[RXQUEUESIZE][RXBUFFSIZE];//for first item if >0 command is inside, 0 after it is proccessed
bool rxBufferMsgReady[RXQUEUESIZE];
uint8_t rxLen,crcH,crcL,readState,rxPtr,rxBufPtr=0;

int gi;//for for loops in switch-case

cell_module moduleList[MODULE_ADDRESS_RANGE_SIZE];
uint8_t modulesCount=0;

//HEATING
bool xHeating;
uint16_t iHeatingEnergyCons;

//StateMachine
uint8_t stateMachineStatus;
unsigned long tmrDelay;
//commands
bool xReqRun,xReqChargeOnly,xReqDisconnect,xReqErrorReset;
bool xEmergencyShutDown;
bool xReadyToSendStatistics;
//aux vars

uint16_t crcMismatchCounter;

RunningMedian temperature_median = RunningMedian(5);

union {
  float val;
  uint8_t buffer[4];
} float_to_bytes;

union {
  uint16_t val;
  uint8_t buffer[2];
} uint16_t_to_bytes;
