
void ConnectToWifi(){
  //zkusíme se připojit k serveru pokud ještě nejsme
    Serial.println("Connecting to wifi");
    WiFi.begin(ssid, password);
    Serial.println("Waiting for connection result..");

    unsigned long wifiConnectStart = millis();
    
    while (WiFi.status() != WL_CONNECTED) {
        // Check to see if
        if (WiFi.status() == WL_CONNECT_FAILED) {
          Serial.println("Failed to connect to WiFi");
        }
    
        delay(500);
        // Only try for 5 seconds.
        if (millis() - wifiConnectStart > 15000) {
          Serial.println("Failed to connect to WiFi");
        }
    }
    Serial.println(F("Connected!"));   
}
void CommWithServer(bool dataForServerReady){
//vytvorime spojeni, posleme data a precteme jestli nejaka neprisla, pak zavreme spojeni
    if (!wifiClient.connect(ipServer, 3666)) {
          Serial.println("Connection to server failed! IP:"+ipServer.toString());
          tcp_conn_failed++;
        } else {

          if(dataForServerReady){
            if(buffer_ptr > 6){
              int cnt = SendToServer(read_buffer+3,buffer_ptr-6);
              if(cnt<=0){
                Serial.println("Write failed!");
              }
            }
          }
          ResetTimer(tmrCheckForData);
          bool payload_received = false;
          Serial.print("Got from server:");
          while(!CheckTimer(tmrCheckForData, 1000UL)){
            delay(100);
            while(wifiClient.available()){
              uint8_t rcv = wifiClient.read();
              payload_received = true;
              bridgeSerial.write(rcv);  
              Serial.print(rcv);
              ResetTimer(tmrCheckForData);
            }
          }   
          if(payload_received){
            tcp_ok_packets ++;
          }
          Serial.println("end.");
        }
        wifiClient.stop();
        //Serial.println("Data sent to server!");
}


int SendToServer(uint8_t d[],uint8_t d_len){
  uint8_t data[6+d_len];
 
  data[0]=111;//start byte
  data[1]=222;//start byte

  data[2]=d_len;//delka

  for(int i=0;i<d_len;i++)
    data[3+i]=d[i];//data
  
  uint16_t crc = CRC16(data+2, d_len+1);

  data[3+d_len]=crc/256;
  data[4+d_len]=crc%256;
  data[5+d_len]=222;//end byte

  return wifiClient.write(data,6+d_len);
}

int SendToArduino(uint8_t d[],uint8_t d_len){
  uint8_t data[6+d_len];
 
  data[0]=111;//start byte
  data[1]=222;//start byte

  data[2]=d_len;//delka

  for(int i=0;i<d_len;i++)
    data[3+i]=d[i];//data
  
  uint16_t crc = CRC16(data+2, d_len+1);

  data[3+d_len]=crc/256;
  data[4+d_len]=crc%256;
  data[5+d_len]=222;//end byte

  return bridgeSerial.write(data,6+d_len);
}