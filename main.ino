/*
LED
pin -> 5v, gnd, 35

Rain drop
pin 25, gnd, 5v

dht11 -> 3v, pin 26

servo1 (front door) -> 5v, pin 14
servo2 (back door)  -> 5v, pin 12

dht11 -> pin 26
*/

//WiFi + WebServer
#include <WiFi.h>
#include <WebServer.h>
// time
#include "time.h"

//RFID
#include <SPI.h>
#include <MFRC522.h>

//servo
#include <ESP32Servo.h>

//DHT11
#include "DHT.h"

//WiFi ssid, password
const char* ssid = "";
const char* password = "";

// time -> ldr
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec= 0;

//web server on port 80 in esp
WebServer server(80);

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




//rain-drop sensor
#define RAIN_DO 25 //pin 25

void handleRainDropInfo(){
  String rainStatus = (digitalRead(RAIN_DO) == LOW) ? "Rain detected..." : "No Rain";
  server.send(200, "text/plain", rainStatus.c_str());
}

bool is_raining;


//RFID module
#define RST_PIN   22  //RST
#define SS_PIN    5   //SDA
#define MISO      19
#define MOSI      23
#define SCK       18

MFRC522 mfrc522(SS_PIN, RST_PIN); 


//servo
#define SERVO1_PIN 14
#define SERVO2_PIN 12
Servo myservo1;
Servo myservo2;

//door controls + rfid
bool id_isValid = false;

//front door open
void frontDoor_handleOn(){
  if(id_isValid){
    if(is_raining){
      myservo1.write(90);
      server.send(200, "text/plain", "Front Door Opened. (Door remains opened for 15s)");
      delay(15000);
      myservo1.write(0);

    } else {
      myservo1.write(90);
      server.send(200, "text/plain", "Front Door Opened...");

    }
    
  } else {
    server.send(200, "text/plain", "Required Door Card...");
  }
}

//front door close
void frontDoor_handleOff(){
    myservo1.write(0);
    server.send(200, "text/plain", "Front Door Closed...");
  
}

//back door open
void backDoor_handleOn(){
  if(id_isValid){
    if(is_raining){
      myservo2.write(90);
      server.send(200, "text/plain", "Back Door Opened. (Door remains opened for 15s)");
      delay(15000);
      myservo2.write(0);

    } else {
      myservo2.write(90);
      server.send(200, "text/plain", "Back Door Opened...");

    }

   
  } else {
    server.send(200, "text/plain", "Required Door Card or Key...");
  }
}

//back door close
void backDoor_handleOff(){
    myservo2.write(0);
    server.send(200, "text/plain", "Back Door Closed...");
  
}

//lock all doors downs
void lockdown_handle(){
  myservo1.write(0);
  myservo2.write(0);
  id_isValid = false;
  server.send(200,"text/plain", "All doors and windows are locked. See you later, Have a nice day!");
}

//dht11
#define DHTPIN 26
#define DHTTYPE DHT11
DHT dht(DHTPIN,DHTTYPE);

const float room_t = 25.0;

void airconditioner_handle(){
  float humidity = dht.readHumidity();
  float t = dht.readTemperature();
  if(t > room_t)
  server.send(200, "text/plain" , "Higher than room temperature. Air conditioner is ON...");
  else
  server.send(200, "text/plain" , "Normal room temperature");
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

  //servo
  myservo1.attach(SERVO1_PIN);
  myservo2.attach(SERVO2_PIN);
  myservo1.write(0);
  myservo2.write(0);


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

  //req mapping
  //use -> http://ip address/ req...
  server.on("/led/on" , handleOn); //led (manual) on
  server.on("/led/off" , handleOff); //led (manual) off
  server.on("/led/status" , handleStatus); //led automated
  server.on("/rain/drop/info",handleRainDropInfo); //rain drop check
  server.on("/front/door/open", frontDoor_handleOn); //front door open (manual)
  server.on("/front/door/close", frontDoor_handleOff); //front door close (manual)
  server.on("/back/door/open", backDoor_handleOn); //front door open (manual)
  server.on("/back/door/close", backDoor_handleOff); //front door close (manual)
  server.on("/lock/down/all", lockdown_handle); //lock all doors
  server.on("/temperature", airconditioner_handle);//dht11

  //server
  server.begin();
  Serial.println("server started");

  //RFID
  SPI.begin(SCK,MISO,MOSI,SS_PIN);	
  mfrc522.PCD_Init();
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  delay(4);	
  mfrc522.PCD_DumpVersionToSerial();	
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

 //dht11
 dht.begin();

}

void loop() {

  //handling client req
  server.handleClient();

  //LDR code
  //time
  struct tm timeinfo;
  bool timeRight = false;

  //led turn on time
  int onTime_h = 21; 
  int onTime_m = 8;

  //light turn of time
  int offTime_h = 21; 
  int offTime_m = 10;

  if(getLocalTime(&timeinfo)){
   
   if((timeinfo.tm_hour >= onTime_h && timeinfo.tm_min >= onTime_m) && (timeinfo.tm_hour <= offTime_h && timeinfo.tm_min <= offTime_m)){
    timeRight = true;
   }
   

  }

  //ldr vlaues
  int ldrstate = analogRead(35);
  int ldrMap = map(ldrstate, 0 ,4095, 0, 100);

  if(wifiLedControl || (ldrMap > 90 && timeRight)){
    digitalWrite(ledPin,HIGH);
  }else{
    digitalWrite(ledPin, LOW);
  }

  delay(1000);

  //dht11 code
  if(dht.readTemperature() > room_t){
    Serial.print("Turning On Air Conditioner ");
    Serial.println(dht.readTemperature());
  } else {
    Serial.println("Turning Off Air Conditioner");
  }
  delay(1000);


  //automatically close doors when raining
  if(digitalRead(RAIN_DO) == LOW){
    delay(15000);
    myservo1.write(0);
    myservo2.write(0);
    is_raining = true;
  } else {
    is_raining = false;

  }

  //RFID
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
		return;
	}

  if ( ! mfrc522.PICC_ReadCardSerial()) {
		return;
	}

  Serial.print("Card UID: ");
	for(byte i = 0; i < mfrc522.uid.size; i++){
		Serial.print(mfrc522.uid.uidByte[1] < 0x10 ? "0" : " ");
		Serial.print(mfrc522.uid.uidByte[1] , HEX);
		Serial.print(" ");

	}
	Serial.println();

  if(
			mfrc522.uid.size == 4 &&
			mfrc522.uid.uidByte[0] == 0xA9 &&
			mfrc522.uid.uidByte[1] == 0x96 &&
			mfrc522.uid.uidByte[2] == 0xC0 &&
			mfrc522.uid.uidByte[3] == 0x01

	){

		Serial.println("Welcome!");
    id_isValid = true;
    myservo1.write(90);
   

	} else {
		Serial.println("Access denied...");
   
	}
	mfrc522.PICC_HaltA();


}
