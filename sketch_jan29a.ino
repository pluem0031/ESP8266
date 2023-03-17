#define BLYNK_TEMPLATE_ID "TMPLRylhR4Dm"
#define BLYNK_TEMPLATE_NAME "QuickShifter Update"

#define BLYNK_FIRMWARE_VERSION        "0.1.0"

#define BLYNK_PRINT Serial
#define APP_DEBUG


#define USE_NODE_MCU_BOARD

#include "BlynkEdgent.h"
#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <SPIFFS.h>
#else
  #include <WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <Hash.h>
  #include <FS.h>
#endif

#include <ESPAsyncWebServer.h>



AsyncWebServer server(80);

int cutoff;
int pitlane;
int i=0;
int j=0;
int dly=0;
//char pass = readFile(SPIFFS, "/wifi_password.txt")
// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid1 = "/ssid1";
const char* password1 = "/pass1";

const char* TEXT_INPUT1 = "HTML_STR_INPUT1";// String type input
const char* INT_INPUT2 = "HTML_INT_INPUT2";// Integer type input
const char* FLOAT_INPUT3 = "HTML_FLOAT_INPUT3"; //Float type input
const char* TPS = "tps";
const char* SSID1 = "ssid1";
const char* PASS1 = "pass1";
const char* SSID2 = "ssid2";
const char* PASS2 = "pass2";
// HTML web page to handle 3 input fields (inputString, inputInt, inputFloat)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Setting QuickShiter</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved Setting");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
    function setpass() {
      alert("WiFi Password is Saved ");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <form action="/get" target="hidden-form">
    Shift Timer(current value %HTML_STR_INPUT1%ms): <input type="number" name="HTML_STR_INPUT1">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    Pitlane Speed (current value %HTML_INT_INPUT2%ms): <input type="number " name="HTML_INT_INPUT2">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    Delay Set (current value %HTML_FLOAT_INPUT3%s): <input type="number " name="HTML_FLOAT_INPUT3">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <form action="/get" target="hidden-form">
    TPS (current value %tps%s): <input type="number " name="tps">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <br><br><br><br><br><br><br><br><br><br>
  <h1>WIFI HOSTSPOT</h1><br><br>
  <form action="/get" target="hidden-form">
    SSID: <input type="text" name="ssid1"><br>
    Password: <input type="text" name="pass1"><br>
    <input type="submit" value="Submit" onclick="setpass()">
  </form><br>
  <h1>WIFI CONNECTION</h1><br><br>
  <form action="/get" target="hidden-form">
    Password: <input type="text" name="pass1">
    <input type="submit" value="Submit" onclick="setpass()">
  </form>
  
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

// Replaces placeholder with stored values
String processor(const String& var){
  //Serial.println(var);
  if(var == "HTML_STR_INPUT1"){
    return readFile(SPIFFS, "/cutoff.txt");
  }
  else if(var == "HTML_INT_INPUT2"){
    return readFile(SPIFFS, "/pitlane.txt");
  }
  else if(var == "HTML_FLOAT_INPUT3"){
    return readFile(SPIFFS, "/delay.txt");
  }
  else if(var == "tps"){
    return readFile(SPIFFS, "/tps.txt");
  }
  return String();
}

void setup() {
  Serial.begin(115200);
  // Initialize SPIFFS
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  

 
  WiFi.softAP(ssid1, password1);


  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET inputString value on <ESP_IP>/get?HTML_STR_INPUT1=<inputMessage>
    if (request->hasParam(TEXT_INPUT1)) {
      inputMessage = request->getParam(TEXT_INPUT1)->value();
      writeFile(SPIFFS, "/cutoff.txt", inputMessage.c_str());
    }
    // GET inputInt value on <ESP_IP>/get?HTML_INT_INPUT2=<inputMessage>
    else if (request->hasParam(INT_INPUT2)) {
      inputMessage = request->getParam(INT_INPUT2)->value();
      writeFile(SPIFFS, "/pitlane.txt", inputMessage.c_str());
    }
    // GET inputFloat value on <ESP_IP>/get?HTML_FLOAT_INPUT3=<inputMessage>
    else if (request->hasParam(FLOAT_INPUT3)) {
      inputMessage = request->getParam(FLOAT_INPUT3)->value();
      writeFile(SPIFFS, "/delay.txt", inputMessage.c_str());
    }
    else if (request->hasParam(TPS)) {
      inputMessage = request->getParam(TPS)->value();
      writeFile(SPIFFS, "/tps.txt", inputMessage.c_str());
    }
    else if (request->hasParam(SSID1)) {
      inputMessage = request->getParam(SSID1)->value();
      writeFile(SPIFFS, "/ssid1.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PASS1)) {
      inputMessage = request->getParam(PASS1)->value();
      writeFile(SPIFFS, "/pass1.txt", inputMessage.c_str());
    }
    else if (request->hasParam(SSID2)) {
      inputMessage = request->getParam(SSID2)->value();
      writeFile(SPIFFS, "/ssid2.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PASS2)) {
      inputMessage = request->getParam(PASS2)->value();
      writeFile(SPIFFS, "/pass2.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();

  Serial.begin(115200);
  delay(100);
  BlynkEdgent.begin();

  pinMode(D6, INPUT_PULLUP);
  pinMode(D4, INPUT_PULLUP);
  pinMode(D2, INPUT_PULLUP);
  pinMode(D5, OUTPUT);
  pinMode(D0, OUTPUT);
  //pinMode(D2, OUTPUT);
  digitalWrite(D5, LOW);
  
}

void loop() {
  BlynkEdgent.run();
  // To access your stored values on inputString, inputInt, inputFloat
  int cutoff_save;
  cutoff= readFile(SPIFFS, "/cutoff.txt").toInt();
  
  
  int pitlane_save; 
  pitlane = readFile(SPIFFS, "/pitlane.txt").toInt();
 
  
  dly = readFile(SPIFFS, "/delay.txt").toInt();
  int tps = readFile(SPIFFS, "/tps.txt").toInt();
  dly*1000;
  
  /*
  EEPROM.write(257, cutoff_save);
  EEPROM.write(256, pitlane_save);

  cutoff==EEPROM.read(257);
  pitlane==EEPROM.read(256);
  */
  
//QuickShift UP Sensor
  int snsrstate=digitalRead(D6);
  if (snsrstate==HIGH) {
    i=0;
    delay(50);
  }
  if (snsrstate==LOW && i==0) {
    
    i+=1;
    digitalWrite(D5, HIGH);
    analogWrite(D0, tps);
    delay(cutoff);
    analogWrite(D0, 0);
    digitalWrite(D5, LOW);
    delay(0);
  }

//QuickShift Down Sensor
  int qsd=digitalRead(D2);
  /*if (qsd==HIGH) {
    i=0;
    delay(50);
  }*/
  if (qsd==LOW && j==0) {
    j+=1;
    analogWrite(D0, tps);
    delay(cutoff);

    analogWrite(D0, 0);
    delay(0);
  }

  int p=digitalRead(D4);
  if (p==LOW) {
    analogWrite(D0, tps);
    delay(pitlane);
    analogWrite(D0, 0);
    delay(dly);
  }
    /*analogWrite(D2, tps);
    delay(pitlane);
    analogWrite(D2, 0);
    delay(dly);

    analogWrite(D0, tps);*/
}