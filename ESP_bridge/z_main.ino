
void loop(){

  wdt_reset();

  ArduinoOTA.handle();

  if (CheckTimer(tmrSendDataToServer, 15000UL)){
      CommWithServer();
      //Serial.println(ntp.formattedTime("%d. %B %Y")); // dd. Mmm yyyy
      //Serial.println(ntp.formattedTime("%A %T")); // Www hh:mm:ss
  }
  if(bridgeSerial.available()>0){
    Serial.println("RECEIVING from arduino");
  }
  while(bridgeSerial.available() > 0) {
    uint8_t rx = bridgeSerial.read();
    Serial.print(rx);
    Serial.print(";");
    
    read_buffer[buffer_ptr++] = rx;
    if (buffer_ptr>=BUFFER_SIZE){
      receive_state = 0;
      buffer_ptr = 0;
    }
    switch(receive_state){
      case 0:
        if(rx==111)
          receive_state++;
        else{
          receive_state=0;
          buffer_ptr = 0;
          serial_invalid_packets1 ++;
        }
      break;
      case 1:
      if(rx==222){
          receive_state++;
      }else{
          receive_state=0;
          buffer_ptr = 0;
          serial_invalid_packets1 ++;
        }
      break;
      case 2:
        expected_len = rx+6; //two start flags + crc two bytes + end flag
        receive_state++;
        ResetTimer(tmrTimeoutReceive);
        break;
      default:
        if(buffer_ptr == expected_len){
          if(rx == 222){
              Serial.println("packet completed, adding to the queue");
              if(queue_ptr_ready+1==queue_ptr_actual || (queue_ptr_ready==QUEUE_SIZE-1&&queue_ptr_actual==0)){// if you are -1 behind actual, it means that buffer is full
                queue_full_packets ++;
              }else{
                memcpy(tx_queue[queue_ptr_ready], read_buffer+3, buffer_ptr-6);
                tx_queue_sizes[queue_ptr_ready]=buffer_ptr-6; // store packet length
                if(++queue_ptr_ready>=QUEUE_SIZE){
                  queue_ptr_ready = 0;
                }
                serial_ok_packets ++;
              }
              receive_state=0;
              buffer_ptr = 0;
          }else{
            receive_state=0;
            buffer_ptr = 0;
            serial_invalid_packets2 ++;
          }
        } else if (buffer_ptr-1 > expected_len){ // TODO add timeout, when nothing received for certain time
            receive_state=0;
            buffer_ptr = 0;   
            serial_invalid_packets2++;       
        }else{
          if (CheckTimer(tmrTimeoutReceive, 100UL)){
            serial_invalid_packets3++;  
          }
        }
      
    }
  }
 
}
