
int Send(uint8_t d[],uint8_t d_len){
  uint8_t data[6+d_len];
 
  data[0]=111;//start byte
  data[1]=222;//start byte

  data[2]=d_len;//delka

  for(int i=0;i<d_len;i++)
    data[3+i]=d[i];//data
  
  uint16_t crc = CRC16(data+2, d_len+1);

  data[3+d_len]=uint8_t(crc/256);
  data[4+d_len]=crc%256;
  data[5+d_len]=222;//end byte

  return wifiClient.write(data,6+d_len);
}
void ProcessReceived(){
  for(int i=0;i<RXQUEUESIZE;i++)
    if(rxBufferMsgReady[i]==true){
      ProcessReceived(rxBuffer[i]);
      rxBufferMsgReady[i]=false;
    }
}
void ProcessReceived(uint8_t data[]){
  switch(data[1]){//podle ID
    case 0:
      LOG(F("RECEIVED REFRESH"));
    break;
    case 199:
      xServerEndPacket = true;
    break;
  }
}

//calculation of CRC16, corresponds to CRC-16/XMODEM on https://crccalc.com/﻿
uint16_t CRC16(uint8_t* bytes, uint8_t length)
{
    const uint16_t generator = 0x1021; /* divisor is 16bit */
    uint16_t crc = 0; /* CRC value is 16bit */

    for (int b=0;b<length;b++)
    {
        crc ^= (uint16_t)(bytes[b] << 8); /* move byte into MSB of 16bit CRC */

        for (int i = 0; i < 8; i++)
        {
            if ((crc & 0x8000) != 0) /* test for MSb = bit 15 */
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

unsigned int CRC16_modbus(unsigned char *buf, int len)
{  
  unsigned int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++)
  {
  crc ^= (unsigned int)buf[pos];    // XOR byte into least sig. byte of crc

  for (int i = 8; i != 0; i--) {    // Loop over each bit
    if ((crc & 0x0001) != 0) {      // If the LSB is set
      crc >>= 1;                    // Shift right and XOR 0xA001
      crc ^= 0xA001;
    }
    else                            // Else LSB is not set
      crc >>= 1;                    // Just shift right
    }
  }

  return crc;
}

void Receive(uint8_t rcv){
  
    switch(readState){
    case 0:
      if(rcv==111){readState=1;}//start token
      break;
    case 1:
      if(rcv==222)readState=2;else { LOG("ERR1");readState=0;}//second start token
      break;
    case 2:
      rxLen = rcv;//length
      if(rxLen>50){readState=0;LOG("ERR2");}else{ readState=3;
        rxPtr=0;
        //vybrat prázdný zásobník
        rxBufPtr=99;
        for(int i=0;i<RXQUEUESIZE;i++)
          if(rxBufferMsgReady[i]==false){
            rxBufPtr=i;
            break;
          }
        if(rxBufPtr==99){
          LOG("FULL BUFF!");
          readState=0;
        }
        rxBuffer[rxBufPtr][rxPtr++] = rxLen;//first stored is length
      }
      break;
    case 3:
      rxBuffer[rxBufPtr][rxPtr++] = rcv;//data
      if(rxPtr>=RXBUFFSIZE || rxPtr>=rxLen+1){
        readState=4;
      }
      break;
    case 4:
      crcH=rcv;//high crc
      readState=5;
      break;
    case 5:
      crcL=rcv;//low crc

      if(CRC16(rxBuffer[rxBufPtr], rxLen+1) == crcL+(uint16_t)crcH*256){//crc check
        readState=6;
      }else {readState=0;LOG("CRC MISMATCH!");}
      break;
    case 6:
      if(rcv==222){//end token
        rxBufferMsgReady[rxBufPtr]=true;//mark this packet as complete
        readState=0;
      }else readState=0;
      break;
    }
}
