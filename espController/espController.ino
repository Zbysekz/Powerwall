
/*
  EspController - esp8266
*/
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include "wifiCredentials.h"

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

bool connectedToWifi = false;
int cntRetryConnect=0,cntSendData=0;
float temp,pressure;
int supplyVoltage;
int i;
bool OTAmode=0;

#define RXBUFFSIZE 60
#define RXQUEUESIZE 5

uint8_t rxBuffer[RXQUEUESIZE][RXBUFFSIZE];
uint8_t rxBufferLen[RXQUEUESIZE];//>0 pokud je v něm příkaz, 0 se nastaví po zpracování
uint8_t rxLen,crcH,crcL,readState,rxPtr,rxBufPtr=0;
uint16_t crcReal;

void otaError(ota_error_t error)
{
  Serial.println(F("OTA Error."));
  switch (error) {
    case OTA_AUTH_ERROR:
      Serial.println("auth. error"); break;
    case OTA_BEGIN_ERROR:
      Serial.println("begin error"); break;
    case OTA_CONNECT_ERROR:
      Serial.println("connect error"); break;
    case OTA_RECEIVE_ERROR:
      Serial.println("receive error"); break;
    case OTA_END_ERROR:
      Serial.println("end error"); break;
    default:
      Serial.println("unknown error");
  }
}

void otaProgress(unsigned int actual, unsigned int maxsize)
{
  if (actual == maxsize - (maxsize / 10)) {
    Serial.println("OTA progress: 90%");
  }
  if (actual == maxsize) {
    Serial.println("OTA progress: 100%");
  }
}

void otaStart()
{
  Serial.println(F("OTA Start."));
}

void otaEnd()
{
  Serial.println(F("OTA End."));
}

void ICACHE_FLASH_ATTR setup() {
 
  Serial.begin(19200);
  Serial.setTimeout(2000);

  // Wait for serial to initialize.
  while(!Serial) { }
  
  Serial.print("\nESP Starting... ");

  Serial.println("IP:");
  Serial.println(ip.toString());

  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);   
      /*
      //Creating own wifi
      WiFi.persistent(false);
      WiFi.mode(WIFI_AP);
      WiFi.softAP("ESP_SETUP");
      */

  Serial.println("Setting up APOTA");
  
  ArduinoOTA.setHostname("APOTA");
  ArduinoOTA.onStart(otaStart);
  ArduinoOTA.onEnd(otaEnd);
  ArduinoOTA.onProgress(otaProgress);
  ArduinoOTA.onError(otaError);
  ArduinoOTA.begin();

  Wire.begin(2,0); // SDA GPIO2, SCL GPIOO -> D4, D3 ------ I2C

  
  Serial.println(F("Setup finished."));
  pinMode(5,INPUT_PULLUP);
  if(!digitalRead(5)){
    Serial.println(F("OTA MODE"));
    OTAmode=1;
  }
}

void loop() {

  if(OTAmode){
    ArduinoOTA.handle();
    return;
  }
  //Try to connect to server if not yet connected
    Serial.println("Connecting to wifi");
    WiFi.begin(ssid, password);
    Serial.println("Waiting for connection result..");

    unsigned long wifiConnectStart = millis();
    
    while (WiFi.status() != WL_CONNECTED) {
        // Check to see if
        if (WiFi.status() == WL_CONNECT_FAILED) {
          Serial.println("Failed to connect to WiFi");
          //ESP.deepSleep(SLEEP_PERIOD);
        }
    
        delay(500);
        // Only try for 15 seconds.
        if (millis() - wifiConnectStart > 15000) {
          Serial.println("Failed to connect to WiFi");
          //ESP.deepSleep(SLEEP_PERIOD);
        }
    }
    Serial.println("Connected!");    


    

   //Create connection, send data and check if any data were received, then close connection
    if (!wifiClient.connect(ipServer, 23)) {
          Serial.println("Connection to server failed! IP:"+ipServer.toString());
        } else {
        uint8_t sbuf[] = {101,int(temp*100)/256, int(temp*100)%256,(int(pressure*100)&0xFF0000)>>16,(int(pressure*100)&0xFF00)>>8,int(pressure*100)&0xFF,(int(supplyVoltage)&0xFF00)>>8,int(supplyVoltage)&0xFF};

        int cnt = Send(sbuf,8);
        if(cnt<=0){
          Serial.println("Write failed!");
        }

        delay(50);
        while(wifiClient.available()){
          uint8_t rcv = wifiClient.read();
          Receive(rcv);
          Serial.write(rcv);
        }
            
        }
        wifiClient.stop();


  delay(1000);
}

int Send(uint8_t d[],uint8_t d_len){
  uint8_t data[6+d_len];
 
  data[0]=111;//start byte
  data[1]=222;//start byte

  data[2]=d_len;//length

  for(int i=0;i<d_len;i++)
    data[3+i]=d[i];//data
  
  int crc = 0;
  for(int i=0;i<d_len+1;i++)
    crc+=data[2+i];

  data[3+d_len]=crc/256;
  data[4+d_len]=crc%256;
  data[5+d_len]=222;//end byte

  return wifiClient.write(data,6+d_len);
}
void ProcessReceived(){
  for(int i=0;i<RXQUEUESIZE;i++)
    if(rxBufferLen[i]>0){
      ProcessReceived(rxBuffer[i]);
      rxBufferLen[i]=0;
    }
}
void ProcessReceived(uint8_t data[]){
  switch(data[0]){//by ID
    case 0:
      Serial.println("Hello world!");
    break;
    case 1:
      Serial.println("Command received:");
     
      Serial.print(data[1],DEC);
      Serial.print(data[2],DEC);
    break;
  }
}
void Receive(uint8_t rcv){
  
    switch(readState){
    case 0:
      if(rcv==111){readState=1;}//start token
      break;
    case 1:
      if(rcv==222)readState=2;else { Serial.write("ERR1");readState=0;}//second start token
      break;
    case 2:
      rxLen = rcv;//length
      if(rxLen>50){readState=0;Serial.write("ERR2");}else{ readState=3;
        rxPtr=0;
        crcReal=rxLen;
        //choose empty stack
        rxBufPtr=99;
        for(int i=0;i<RXQUEUESIZE;i++)
          if(rxBufferLen[i]==0)
            rxBufPtr=i;
        if(rxBufPtr==99){
          Serial.write("FULL BUFF!");
          readState=0;
        }
      }
      break;
    case 3:
      rxBuffer[rxBufPtr][rxPtr++] = rcv;//data
      crcReal+=rcv;
      if(rxPtr>=RXBUFFSIZE || rxPtr>=rxLen){
        readState=4;
      }
      break;
    case 4:
      crcH=rcv;//high crc
      readState=5;
      break;
    case 5:
      crcL=rcv;//low crc

      if(crcReal == crcL+(uint16_t)crcH*256){//crc check
        readState=6;
      }else {readState=0;Serial.write("CRC MISMATCH!");}
      break;
    case 6:
      if(rcv==222){//end token
        rxBufferLen[rxBufPtr]=rxLen;//set this packet to finished
        readState=0;
      }else readState=0;
      break;
    }
}
