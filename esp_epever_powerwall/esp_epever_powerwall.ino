/* 
  NodeMCU - powerwall communication for Epever MPPT charge controller
  Compile as GENERIC ESP8266
*/
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <U8x8lib.h> //OLED display https://github.com/olikraus/u8g2 (U8G2 in library manager)
#include <Wire.h>
#include "credentials/credentials.h" // separate file for credentials

#define DEBUG false
#define LOG(x); {if(DEBUG)Serial.println(x);}

U8X8_SSD1306_128X64_NONAME_HW_I2C display; //U8X8_SSD1306_128X64_NONAME_HW_I2C

// the IP address for the shield:
IPAddress ip(192, 168, 0, 15);  
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

IPAddress ipServer(192, 168, 0, 3);  

WiFiClient wifiClient;
bool connectedToWifi = false,connectedToServer = false,xServerEndPacket;
int cntRetryConnect=0,cntSendData=0,cntTimeoutIncomeData=0,tmrActiveComm=0;
uint32_t tmrCommTimeout;

#define RXBUFFSIZE 40
#define RXQUEUESIZE 5

#define SERIAL_RX_BUFF_SIZE 30

uint8_t rxBuffer[RXQUEUESIZE][RXBUFFSIZE];
bool rxBufferMsgReady[RXQUEUESIZE];//true if cmd is inside, false is set after its processed
uint8_t rxLen,crcH,crcL,readState,rxPtr,rxBufPtr=0;
bool displayOk,errorNotValidData;
uint32_t tmrSendToServer,tmrRequest,tmrTimeout, tmrNotValidData;
bool commOk;


//epever
uint8_t serial_rxPtr,epeverStatus;
uint8_t serial_rcvBuff[SERIAL_RX_BUFF_SIZE];
bool epeverDataValid,epeverReadReq;
uint16_t rxValue;
uint16_t tempRxValue;
uint16_t crc,timeoutReq,crcErrorCnt;
uint32_t tmrEpeverRead;
uint8_t epeverRegPtr,epeverRegPtr2;

//measured data from EPsolar regulator
struct epever_data_t{
uint32_t batteryPower;
uint16_t batVoltage, temperature,generatedEnergy;
uint16_t batteryStatus, chargerStatus;
uint8_t comm_status; // 0 for data not yet read, 1 ok read all registers, 2 timeout comm
};

#define EPEVER_MODULES 2

epever_data_t epever_data[EPEVER_MODULES];
uint8_t selected_device;
uint8_t epever_statuses;
uint8_t cnt_single_epever;

#define EPEVER_READ_REG_LEN 8
uint16_t epeverReadRegisters[EPEVER_READ_REG_LEN] = {0x3104,//Battery voltage
                                  0x3110,//Battery temperature (but it's temperature inside garage)
                                  0x3102,0x3103,// Battery power
                                  0x330C,0x330D,//Generated energy today
                                  0x3200,//battery status
                                  0x3201};//charger status

#define EPEVER_READ_REG_WORD_LEN 6
bool epeverReadRegisters_word[EPEVER_READ_REG_WORD_LEN] = {false,false,true,true,false,false};

void otaError(ota_error_t error)
{
  LOG(F("OTA Error."));
  switch (error) {
    case OTA_AUTH_ERROR:
      LOG("1"); break;
    case OTA_BEGIN_ERROR:
      LOG("2"); break;
    case OTA_CONNECT_ERROR:
      LOG("3"); break;
    case OTA_RECEIVE_ERROR:
      LOG("4"); break;
    case OTA_END_ERROR:
      LOG("5"); break;
  }
  LOG("6");
}

void otaProgress(unsigned int actual, unsigned int maxsize)
{
  if (actual == maxsize - (maxsize / 10)) {
    LOG(F("OTA progress: 90%"));
  }
  if (actual == maxsize) {
    LOG(F("OTA progress: 100%"));
  }
}

void otaStart()
{
  LOG(F("OTA Start."));
}

void otaEnd()
{
  LOG(F("OTA End."));
}

void ICACHE_FLASH_ATTR setup() {
  Serial.begin(114000);//115200
  LOG(F("\nStarting... "));
  delay(100);

  LOG(F("IP:"));
  LOG(ip.toString());

  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);   
     /*
      //vytvarime vlastni testovaci
      WiFi.persistent(false);
      WiFi.mode(WIFI_AP);
      WiFi.softAP("ESP_SETUP");
      */

  LOG(F("Setting up APOTA"));
  
  ArduinoOTA.setHostname("APOTA");
  ArduinoOTA.onStart(otaStart);
  ArduinoOTA.onEnd(otaEnd);
  ArduinoOTA.onProgress(otaProgress);
  ArduinoOTA.onError(otaError);
  ArduinoOTA.begin();

  Wire.setTimeout(1000);  //1000ms timeout
  Wire.setClock(100000);  //100khz
  //Wire.begin(5,0); // I2C -- SDA GPIO5, SCL GPIOO /if we are using LCD it is already initialized  
  
  if(display.begin()){// for ESP display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
    // Clear the buffer.
    display.clearDisplay();
    display.setFont(u8x8_font_chroma48medium8_r);
    displayOk = true;
  }

  pinMode(2,OUTPUT);
  digitalWrite(2,true); // LED OFF
  
  LOG(F("Setup finished."));

  selected_device = 1;
  cnt_single_epever = 0;
  
}
