
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <SoftwareSerial.h>

#include "credentials/credentials.h" // separate file for credentials

#include <EEPROM.h>

//webserver from tutorial https://randomnerdtutorials.com/esp32-esp8266-input-data-html-form/
