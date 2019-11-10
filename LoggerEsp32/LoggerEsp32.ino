#include <WiFi.h>
#include <DHT.h>
#include <HTTPSRedirect.h>
#include "time.h"
#include "arduino_secrets.h"

#define DHTPIN 2 //Temp/Hum sensor pin
#define DHTTYPE DHT11     // DHT 11
#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x)     Serial.print(x)
#define DEBUG_PRINTDEC(x)  Serial.print(x, DEC)
#define DEBUG_PRINTLN(x)   Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTDEC(x)
#define DEBUG_PRINTLN(x)
#endif



//From arduino_secrets.h
String GAS_ID = GoogleAppScript_ID; //Identifies Google App Script
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
const char* host = "script.google.com";
const int httpsPort = 443;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
long int logInterval = 900000L;
int timerGoogle;
byte hum;
float temp;
DHT dht(DHTPIN, DHTTYPE);
WiFiServer server(80);
HTTPSRedirect* client = nullptr;


//Reads data from DHT sensor & Calls function to keep stats update
void readData() {
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  if (isnan(hum) || isnan(temp)) {
    DEBUG_PRINTLN(F("DHT Read Fail. Fake Data Used"));
    hum = 10;
    temp = 1;
    return;
  }
}

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void sendData() {
  //lcdClearLine(3);
  readData();
  String url = String("/macros/s/") + GAS_ID + "/exec?temp=" + temp + "&hum=" + hum;


  if (WiFi.status() != WL_CONNECTED)  {
    DEBUG_PRINTLN(F("No connection "));
    return;
  }

  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");

  Serial.print("Connecting to ");
  Serial.println(host);

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  if (!flag) {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    return;
  }

}




void setup()
{
  Serial.begin(115200);
  pinMode(DHTPIN, INPUT);

  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
}

void loop()
{
  delay(1000);
  printLocalTime();
  sendData();
  delay(10000);
}
