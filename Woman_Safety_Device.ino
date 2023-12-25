#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <TinyGPS++.h>
#include <ThingSpeak.h>

// Define the pins for communication with the modules.
const int gpsRxPin = D1;
const int gpsTxPin = D2;
const int gsmRxPin = D6;
const int gsmTxPin = D5;
const int SOSpin = D8;

const char* ssid = "windows";
const char* password = "sonukaphone";
const char* apiKey = "KFD1H0D3KEZ3W60J";
const unsigned long channelID = 2337349;
const String PHONE = "+917634859782";

float Latitude, Longitude;
String LatitudeString, LongitudeString;
int responseStatus = 0; // Global variable to hold the response status
unsigned long previousMillis = 0; // Store the last time data was sent
const unsigned long interval = 300000; // Interval in milliseconds (5 minute)

TinyGPSPlus gps;
WiFiClient  client;
SoftwareSerial gpsSerial(gpsTxPin, gpsRxPin);  // Create a SoftwareSerial object for GPS communication
SoftwareSerial gsmSerial(gsmTxPin, gsmRxPin);  //// Create a SoftwareSerial object for GSM communication

void updateGsmSerial() {
  delay(500);
  while (Serial.available()) {
    gsmSerial.write(Serial.read());  //Forward what Serial received to gsm Serial Port
  }
  delay(500);
  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read());  //Forward what gsm Serial received to Serial Port
  }
}

 // Pass 0 or 1 for field3
void sendToThingSpeak(int field3Value) {
  Serial.println("Sending data to ThingSpeak");
   if(WiFi.status() != WL_CONNECTED){
    return;
   }
  // Set values to fields
  ThingSpeak.setField(1, LatitudeString);
  ThingSpeak.setField(2, LongitudeString);
  ThingSpeak.setField(3, field3Value);

  // Write to ThingSpeak
  responseStatus = ThingSpeak.writeFields(channelID, apiKey);

  delay(100); // Send data to ThingSpeak(adjust as needed)
}

void setup() {
  // put your setup code here, to run once:
  LatitudeString = "30.858735", LongitudeString = "75.861335";
  Serial.begin(9600);
  Serial.println("Initializing project");
  pinMode(SOSpin, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  gpsSerial.begin(9600);
  gsmSerial.begin(9600);
  WiFi.begin(ssid, password);
  delay(10000);

  // Setting up SIM800L
  gsmSerial.println("AT+CSMP=17,167,0,0");  //Once the handshake test is successful, it will back to OK
  updateGsmSerial();

  ThingSpeak.begin(client);

  Serial.println("Entering loop");
}

void loop() {
  // put your main code here, to run repeatedly:
  int blackSwitch = digitalRead(SOSpin);
  Serial.println(blackSwitch);
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      if (gps.location.isValid()) {
        Latitude = gps.location.lat();
        LatitudeString = String(Latitude, 6);
        Longitude = gps.location.lng();
        LongitudeString = String(Longitude, 6);
      }
      delay(100);
    }
  }

  if (blackSwitch == HIGH) {
    String mapUrl = "Help! "+"http://maps.google.com/maps?&z=15&mrt=yp&t=k&q=" + LatitudeString + "+" + LongitudeString;
    digitalWrite(LED_BUILTIN, LOW);
    gsmSerial.println("AT");  // checking connectivity
    updateGsmSerial();
    gsmSerial.println("AT+CMGF = 1");  
    updateGsmSerial();
    // gsmSerial.println("AT+CMGDA=\"DEL ALL\"");  //Delete storage for sms
    // updateGsmSerial();
    delay(10000);
    gsmSerial.print("AT+CMGF=1\r");
    delay(1000);
    gsmSerial.print("AT+CMGS=\"" + PHONE + "\"\r");
    delay(1000);
    gsmSerial.print(mapUrl);
    delay(100);
    gsmSerial.write(0x1A);  //ascii code for ctrl-26
    delay(1000);
    updateGsmSerial();
    delay(1000);
    Serial.println("SMS Sent Successfully.");
    sendToThingSpeak(1);
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval || responseStatus!=200) {
    sendToThingSpeak(0);
    previousMillis = currentMillis;
  }

  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
}