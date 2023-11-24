#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <TinyGPS++.h>

// Define the pins for communication with the modules.
const int gpsRxPin = D1;
const int gpsTxPin = D2;
const int gsmRxPin = D6;
const int gsmTxPin = D5;
const int blackPin = D8;  //for normal case

const char* ssid = "windows";
const char* password = "sonukaphone";
const String PHONE = "+919334463718";

float Latitude, Longitude;
int year, month, date, hour, minute, second;
String DateString, TimeString, LatitudeString, LongitudeString;

TinyGPSPlus gps;
SoftwareSerial gpsSerial(gpsTxPin, gpsRxPin);  // Create a SoftwareSerial object for GPS communication
SoftwareSerial gsmSerial(gsmTxPin, gsmRxPin);  //// Create a SoftwareSerial object for GSM communication

WiFiServer server(80);

void updateGsmSerial() {
  delay(500);
  while (Serial.available()) {
    gsmSerial.write(Serial.read());  //Forward what Serial received to gsm Serial Port
  }
  while (gsmSerial.available()) {
    Serial.write(gsmSerial.read());  //Forward what gsm Serial received to Serial Port
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Initializing project");
  pinMode(blackPin, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  gpsSerial.begin(9600);
  gsmSerial.begin(9600);
  WiFi.begin(ssid, password);
  delay(10000);
  if (WiFi.status() == WL_CONNECTED) {
    server.begin();
    Serial.println("Server started");
    Serial.println(WiFi.localIP());
  }
  // Setting up SIM800L
  gsmSerial.println("AT");  // checking connectivity
  updateGsmSerial();
  gsmSerial.println("AT+COPS?");  // checking connectivity
  updateGsmSerial();
  gsmSerial.println("AT+CBC");  // checking power
  updateGsmSerial();
  gsmSerial.println("AT+CREG?");  //Once the handshake test is successful, it will back to OK
  updateGsmSerial();
  gsmSerial.print("AT+CMGF=1\r");
  delay(1000);

  Serial.println("Entering loop");
}

void loop() {
  // put your main code here, to run repeatedly:
  int blackSwitch = digitalRead(blackPin);
  Serial.println(blackSwitch);
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      if (gps.location.isValid()) {
        Latitude = gps.location.lat();
        LatitudeString = String(Latitude, 6);
        Longitude = gps.location.lng();
        LongitudeString = String(Longitude, 6);
      }

      if (gps.date.isValid()) {
        DateString = "";
        date = gps.date.day();
        month = gps.date.month();
        year = gps.date.year();

        if (date < 10)
          DateString = '0';
        DateString += String(date);

        DateString += " / ";

        if (month < 10)
          DateString += '0';
        DateString += String(month);
        DateString += " / ";

        if (year < 10)
          DateString += '0';
        DateString += String(year);
      }

      if (gps.time.isValid()) {
        TimeString = "";
        hour = gps.time.hour() + 5;  //adjust UTC
        minute = gps.time.minute();
        second = gps.time.second();

        if (hour < 10)
          TimeString = '0';
        TimeString += String(hour);
        TimeString += " : ";

        if (minute < 10)
          TimeString += '0';
        TimeString += String(minute);
        TimeString += " : ";

        if (second < 10)
          TimeString += '0';
        TimeString += String(second);
      }
    }
    WiFiClient client = server.available();
    if (!client) {
      return;
    }

    //Response
    String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n <!DOCTYPE html> <html> <head> <title>NEO-6M GPS Readings</title> <style>";
    s += "table, th, td {border: 1px solid blue;} </style> </head> <body> <h1  style=";
    s += "font-size:300%;";
    s += " ALIGN=CENTER>NEO-6M GPS Readings</h1>";
    s += "<p ALIGN=CENTER style="
         "font-size:150%;"
         "";
    s += "> <b>Location Details</b></p> <table ALIGN=CENTER style=";
    s += "width:50%";
    s += "> <tr> <th>Latitude</th>";
    s += "<td ALIGN=CENTER >";
    s += LatitudeString;
    s += "</td> </tr> <tr> <th>Longitude</th> <td ALIGN=CENTER >";
    s += LongitudeString;
    s += "</td> </tr> <tr>  <th>Date</th> <td ALIGN=CENTER >";
    s += DateString;
    s += "</td></tr> <tr> <th>Time</th> <td ALIGN=CENTER >";
    s += TimeString;
    s += "</td>  </tr> </table> ";
    if (gps.location.isValid()) {
      s += "<p align=center><a style="
           "color:RED;font-size:125%;"
           " href="
           "http://maps.google.com/maps?&z=15&mrt=yp&t=k&q=";
      s += LatitudeString;
      s += "+";
      s += LongitudeString;
      s += ""
           " target="
           "_top"
           ">Click here</a> to open the location in Google Maps.</p>";
    }
    s += "</body> </html> \n";
    client.print(s);
    delay(100);
  }

  if (blackSwitch == HIGH) {
    String mapUrl = "http://maps.google.com/maps?&z=15&mrt=yp&t=k&q=" + LatitudeString + "+" + LongitudeString;
    digitalWrite(LED_BUILTIN, LOW);
    gsmSerial.print("AT+CMGS=\"" + PHONE + "\"\r");
    delay(1000);
    gsmSerial.print(mapUrl);
    delay(100);
    gsmSerial.write(0x1A);  //ascii code for ctrl-26 //gsmSerial.println((char)26); //ascii code for ctrl-26
    delay(1000);
    Serial.println("SMS Sent Successfully.");
    delay(10000);
  }
  digitalWrite(LED_BUILTIN, HIGH);
}
