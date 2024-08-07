

//Show error if not targetting ATTINY85
#if !(defined(__AVR_ATtiny85__))
#error Written for ATTINY85/V chip
#endif

#if !(F_CPU == 8000000)
#error Processor speed should be 8Mhz internal
#endif

#include <TinyWireS.h>

#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

//LED light patterns
#define GREEN_LED_PATTERN_STANDARD 0
#define GREEN_LED_PATTERN_BYPASS B01101100
#define GREEN_LED_PANIC B01010101
#define GREEN_LED_PATTERN_UNCONFIGURED B11101111

//Where in EEPROM do we store the configuration
#define EEPROM_CHECKSUM_ADDRESS 60
#define EEPROM_CONFIG_ADDRESS 90

#define MIN_BYPASS_VOLTAGE 300U //x10mV
#define MAX_BYPASS_VOLTAGE 420U //x10mV

//Number of TIMER1 cycles between voltage reading checks (240 = approx 30 seconds)
#define BYPASS_COUNTER_MAX 240

//Default i2c SLAVE address (used for auto provision of address)
#define DEFAULT_SLAVE_ADDR 21

//Configured cell modules use i2c addresses 24 to 36 (12S)
#define SLAVE_ADDR_START_RANGE 24
#define SLAVE_ADDR_END_RANGE SLAVE_ADDR_START_RANGE + 12

//Number of times we sample and average the ADC for voltage
#define OVERSAMPLE_LOOP 16
//Number of voltage readings to take before we take a temperature reading
#define TEMP_READING_LOOP_FREQ 16

#define MAX_BURN_CYCLES 1000000UL // protection for not burning too long

//If we receive a cmdByte with BIT 6 set its a command byte so there is another byte waiting for us to process
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

#define READOUT_voltage 10
#define READOUT_temperature 11
#define READOUT_voltage_calibration 12
#define READOUT_temperature_calibration 13
#define READOUT_raw_voltage 14
#define READOUT_error_counter 15
#define READOUT_bypass_state 16
#define READOUT_bypass_voltage_measurement 17
#define READOUT_burningCounter 18
#define READOUT_voltage_calibration2 19
#define READOUT_temperature_calibration2 20
#define READOUT_CRC 21

volatile bool skipNextADC = false;
volatile uint16_t voltageBuff[OVERSAMPLE_LOOP];

volatile uint16_t tempSensorValue = 0;
volatile uint8_t voltageBufIdx;
volatile bool voltageBufferReady = 0;
volatile uint8_t tempReadingCnt = 0;
volatile uint8_t cmdByte = 0;
volatile uint8_t i2cTmr = 255;
volatile uint32_t cntBurnCycles = 0;

volatile bool ledFlash = false;
volatile bool badConfiguration = false;
volatile uint8_t green_pattern = GREEN_LED_PATTERN_UNCONFIGURED;


volatile uint16_t actualTemperature;
volatile uint16_t voltageMeasurement = 0;
volatile uint16_t voltageMeasurement_bypass = 0;
volatile uint16_t last_raw_adc = 0;
volatile uint16_t targetBypassVoltage = 0;
volatile uint8_t bypassCnt = 0;
volatile uint16_t iBurningCounter=0;
volatile boolean bypassEnabled = false;
volatile uint8_t CRC_tempVolt;


boolean inPanicMode = false;
uint16_t error_counter = 0;

//Default values
struct cell_module_config {
  // 7 bit slave I2C address
  uint8_t SLAVE_ADDR = DEFAULT_SLAVE_ADDR;
  // Calibration factor for voltage readings
  float voltageCalibration = 0.447;
  float voltageCalibration2 = 0.0;
  // Calibration factor for temp readings
  float tempSensorCalibration = 0.35;
  float tempSensorCalibration2 = 0.0;
};

static cell_module_config currentConfig;


//unions for byte conversion
union {
  float val;
  uint8_t b[4];
} float_to_bytes;

union {
  uint16_t val;
  uint8_t b[2];
} uint16_t_to_bytes;
