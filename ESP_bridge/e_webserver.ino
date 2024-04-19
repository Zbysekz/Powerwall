

// HTML web page to handle 3 input fields (input1, input2, input3)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP bridge for PowerWall</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv='refresh' content='30'> 
  </head><body>
  <h1>ESP bridge for PowerWall</h1>
  <form action="/" method="post" accept-charset=utf-8>
    <input type="submit" name="reboot" value="reboot">
  </form><br>
  <br>)rawliteral";
  
 
void notFound(AsyncWebServerRequest *request);

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void ServerSetup(){
  // Send web page with input fields to client
  server.on("/", HTTP_POST | HTTP_GET, [](AsyncWebServerRequest *request){

    // Display current state    
    char result[2000];
    strcpy(result,index_html);

    if(request->method() == HTTP_POST){
      // check for commands -------------------------------------------
      String inputMessage;
      String inputParam;
      // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
      if (request->hasParam("reboot", true)) {
        Serial.println("Reboting per webserver request...");
        ESP.restart(); //also could use .reset() which is more hard
      }
    }
    // -----------------------------------------------------------------

    
        
    strcat(result,("<p>ok packets: " + String(ok_packets) + "</p>").c_str());
    strcat(result,("<p>invalid1 packets: " + String(invalid_packets1) + "</p>").c_str());
    strcat(result,("<p>invalid2 packets: " + String(invalid_packets2) + "</p>").c_str());
    strcat(result,("<p>invalid3 packets: " + String(invalid_packets3) + "</p>").c_str());
    strcat(result,("<p>Free heap: " + String(ESP.getFreeHeap()) +" B</p>").c_str());

    long millisecs = millis();
    int systemUpTimeMn = int((millisecs / (1000 * 60)) % 60);
    int systemUpTimeHr = int((millisecs / (1000 * 60 * 60)) % 24);
    int systemUpTimeDy = int((millisecs / (1000 * 60 * 60 * 24)) % 365);

    strcat(result,("<p>Uptime: "+ String(systemUpTimeDy) + "days " + String(systemUpTimeHr) + "h "+ String(systemUpTimeMn) +"min</p>").c_str());

    strcat(result, "</body></html>");
    request->send_P(200, "text/html", result);
    
  });
  server.onNotFound(notFound);
  server.begin();
}
