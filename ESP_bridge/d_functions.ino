//function for creating periodic block code
//returns true if period has passed, resetting the timer
bool CheckTimer(unsigned long &timer, const unsigned long period){
   if((unsigned long)(millis() - timer) >= period){//protected from rollover by type conversion
    timer = millis();
    return true;
   }else{
    return false;
   }
}
void ResetTimer(unsigned long &timer){
  timer = millis();
}

String formatVal(float val, int minWidth=5, int precision=1);
   
String formatVal(float val, int minWidth, int precision){
   char buff[10];
   dtostrf(val, minWidth, precision, buff);
   String str(buff);
   return str;
}

void StoreValue(uint8_t *arr, float val, uint8_t &ptr){
   arr[ptr++] = (int)(val*10)/256;
   arr[ptr++] = (int)(val*10)%256;
}

uint8_t ReadValue(uint8_t *arr, uint8_t &ptr){
  uint8_t res = (arr[ptr]*256 + arr[ptr+1])/10.0;
  ptr+=2;
  return res;
}

//calculation of CRC16, corresponds to CRC-16/XMODEM on https://crccalc.com/﻿
uint16_t CRC16(uint8_t* bytes, uint8_t length)
{
    const uint16_t generator = 0x1021; // divisor is 16bit 
    uint16_t crc = 0; // CRC value is 16bit 

    for (int b=0;b<length;b++)
    {
        crc ^= (uint16_t)(bytes[b] << 8); // move byte into MSB of 16bit CRC

        for (int i = 0; i < 8; i++)
        {
            if ((crc & 0x8000) != 0) // test for MSb = bit 15
            {
                crc = (uint16_t)((crc << 1) ^ generator);
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}
