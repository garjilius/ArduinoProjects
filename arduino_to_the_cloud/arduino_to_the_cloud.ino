

//-----------------------------------------------
// Author: Trieu Le
// Email: lethanhtrieuk36@gmail.com
// Publish date: 29-Oct-2017
// Description: This code for demonstration send data from ESP8266 into Google Spreadsheet
// Modifyed by Moz for Youtube changel logMaker360 for this video: https://youtu.be/fS0GeaOkNRw 24-02-2018
// update ssid, password and GAS_ID ->  AKfycbyJgvc9Kg3UzkkN_IDy4rPSexJGnunSMjsVoP5gS6J2tvXId6MM
//-----------------------------------------------
#include <DHT.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include<DHT.h>
#define DHTPIN 2
#define DHTTYPE DHT11 
 
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Garjilius";    // name of your wifi network!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const char* password = "garjiliusnet27";     // wifi pasword !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const char* host = "script.google.com";
const int httpsPort = 443;
// Use WiFiClientSecure class to create TLS connection
WiFiClientSecure client;
// SHA1 fingerprint of the certificate, don't care with your GAS service
const char* fingerprint = "46 B2 C3 44 9C 59 09 8B 01 B6 F8 BD 4C FB 00 74 91 2F EF F6";
String GAS_ID = "AKfycbyJgvc9Kg3UzkkN_IDy4rPSexJGnunSMjsVoP5gS6J2tvXId6MM"; 	// Replace by your GAS service id           !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
int it;
int ih;
void setup() 
{
  
  dht.begin();  // sensor
  Serial.begin(115200); //Serial
  Serial.println();

  //connecting to internet
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
 

}

void loop() 
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  Serial.print("Temp = ");
  Serial.print(t);
  Serial.print(" HUM= ");
  Serial.println(h);
   it = (int) t;
   ih = (int) h;
  sendData(it, ih);
 
 delay(2000);
}

// Function for Send data into Google Spreadsheet
void sendData(int tem, int hum, int mov)
{
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  if (client.verify(fingerprint, host)) {
  Serial.println("certificate matches");
  } else {
  Serial.println("certificate doesn't match");
  }
  String string_temperature =  String(tem, DEC); 
  String string_humidity =  String(hum, DEC); 
  String url = "/macros/s/" + GAS_ID + "/exec?temperature=" + string_temperature + "&humidity=" + string_humidity;
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
         "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
  String line = client.readStringUntil('\n');
  if (line == "\r") {
    Serial.println("headers received");
    break;
  }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
  Serial.println("esp8266/Arduino CI successfull!");
  } else {
  Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");
} 
