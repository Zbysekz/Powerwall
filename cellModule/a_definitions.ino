

//Show error is not targetting ATTINY85
#if !(defined(__AVR_ATtiny85__))
#error Written for ATTINY85/V chip
#endif

#if !(F_CPU == 8000000)
#error Processor speed should be 8Mhz internal
#endif

#include <USIWire.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

// __DATE__

//LED light patterns
#define GREEN_LED_PATTERN_STANDARD 0
#define GREEN_LED_PATTERN_BYPASS B01101100
#define GREEN_LED_PANIC B01010101
#define GREEN_LED_PATTERN_UNCONFIGURED B11101111

//Where in EEPROM do we store the configuration
#define EEPROM_CHECKSUM_ADDRESS 0
#define EEPROM_CONFIG_ADDRESS 20

#define MIN_BYPASS_VOLTAGE 3000U
#define MAX_BYPASS_VOLTAGE 4200U

//Number of TIMER1 cycles between voltage reading checks (240 = approx 30 seconds)
#define BYPASS_COUNTER_MAX 240

//Default i2c SLAVE address (used for auto provision of address)
#define DEFAULT_SLAVE_ADDR 21

//Configured cell modules use i2c addresses 24 to 48 (24S)
#define DEFAULT_SLAVE_ADDR_START_RANGE 24
#define DEFAULT_SLAVE_ADDR_END_RANGE DEFAULT_SLAVE_ADDR_START_RANGE + 24

//Number of times we sample and average the ADC for voltage
#define OVERSAMPLE_LOOP 16

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


#define read_voltage 10
#define read_temperature 11
#define read_voltage_calibration 12
#define read_temperature_calibration 13
#define read_raw_voltage 14
#define read_error_counter 15
#define read_bypass_enabled_state 16
#define read_bypass_voltage_measurement 17
#define read_load_resistance 18

volatile bool skipNextADC = false;
volatile uint8_t green_pattern = GREEN_LED_PATTERN_STANDARD;
volatile uint16_t analogVal[OVERSAMPLE_LOOP];
volatile uint16_t temperature_probe = 0;
volatile uint8_t analogValIndex;
volatile uint8_t buffer_ready = 0;
volatile uint8_t reading_count = 0;
volatile uint8_t cmdByte = 0;

volatile uint8_t last_i2c_request = 255;
volatile uint16_t VCCMillivolts = 0;
volatile uint16_t ByPassVCCMillivolts = 0;
volatile uint16_t last_raw_adc = 0;
volatile uint16_t targetByPassVoltage = 0;
volatile uint8_t ByPassCounter = 0;
volatile boolean ByPassEnabled = false;
volatile boolean flash_green = false;

uint16_t error_counter = 0;

bool flashLed;

//Number of voltage readings to take before we take a temperature reading
#define TEMP_READING_LOOP_FREQ 16

//Default values
struct cell_module_config {
  // 7 bit slave I2C address
  uint8_t SLAVE_ADDR = DEFAULT_SLAVE_ADDR;
  // Calibration factor for voltage readings
  float VCCCalibration = 4.430;
  // Calibration factor for temp readings
  float TemperatureCalibration = 1.080;
  // Resistance of bypass load
  float LoadResistance = 10.0;
};

static cell_module_config myConfig;


boolean inPanicMode = false;

uint8_t previousLedState = 0;

union {
  float val;
  uint8_t b[4];
} float_to_bytes;

union {
  uint16_t val;
  uint8_t b[2];
} uint16_t_to_bytes;

uint16_t readUINT16() {
  uint16_t_to_bytes.b[0] = Wire.read();
  uint16_t_to_bytes.b[1] = Wire.read();
  return uint16_t_to_bytes.val;
}
