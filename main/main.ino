/*
LED
pin -> 5v, gnd, 14

1. manually controlled through wifi
2. automatically light up under low light and at set time
3. when time is right

Rain drop
pin 25, gnd, 5v
check whether rain is detected when a req is sent by the client
*/


#include <WiFi.h>
#include <WebServer.h>
// time
#include "time.h"

//WiFi ssid, password
const char* ssid = "";
const char* password = "";

// time -> ldr
//led
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec= 0;

//web server on port 80 in esp
WebServer server();

//led 
int ledPin = 2;
int ldr = 35;
int ldrstate;

//led -> state var
bool wifiLedControl = false;

void handleOn(){
wifiLedControl = true;
server.send(200,"text/plain", "LED ON");

}

void handleOff(){
wifiLedControl = false;
server.send(200,"text/plain","LED OFF");

}

void handleStatus(){
  if(digitalRead(ledPin)){
    server.send(200, "text/plain" , "LED is ON");
  } else {
    server.send(200, "text/plain" , "LED is OFF");
  }
}




//rain-drop
#define RAIN_DO 25 //pin 25

void handleRainDropInfo(){
  String rainStatus = (digitalRead(RAIN_DO) == LOW) ? "Rain detected..." : "No Rain";
  server.send(200, "text/plain", rainStatus.c_str());
}


void setup() {

  Serial.begin(115200);
  //led pin
  pinMode(ledPin, OUTPUT);
  //lrd process
  pinMode(ldr, INPUT);
  //rain sensor
  pinMode(RAIN_DO, INPUT);
  // digitalWrite(ledPin,LOW);


  //wifi connection
  WiFi.begin(ssid,password);
  Serial.print("connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("connected");
  Serial.println(WiFi.localIP());

  //local time zone setting
  configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);

  server.on("/led/on" , handleOn); //led (manual) on
  server.on("/led/off" , handleOff); //led (manual) off
  server.on("/led/status" , handleStatus); //led automated
  server.on("/rain/drop/info",handleRainDropInfo); //rain drop check

  server.begin();
  Serial.println("server started");


}

void loop() {

  //handling client req
  server.handleClient();

  //time
  struct tm timeinfo;
  bool timeRight = false;

  //led turn on time
  int onTime_h = 14; 
  int onTime_m = 33;

  //light turn of time
  int offTime_h = 14; 
  int offTime_m = 38;

  if(getLocalTime(&timeinfo)){
   
   if((timeinfo.tm_hour >= onTime_h && timeinfo.tm_min >= onTime_m) && (timeinfo.tm_hour <= offTime_h && timeinfo.tm_min <= offTime_m)){
    timeRight = true;
   }
   

  }

  //ldr vlaues
  int ldrstate = analogRead(35);
  int ldrMap = map(ldrstate, 0 ,4095, 0, 100);
  Serial.println(ldrMap);


  if(wifiLedControl || (ldrMap > 90 && timeRight)){
    digitalWrite(ledPin,HIGH);
  }else{
    digitalWrite(ledPin, LOW);
  }

  delay(1000);




}
