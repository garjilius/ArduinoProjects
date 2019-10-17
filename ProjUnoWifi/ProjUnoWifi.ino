#include <DHT.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <BlynkSimpleWiFiNINA.h>
#include <virtuabotixRTC.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header //

#define DHTPIN 2
#define IRPIN 9
#define MOVLED 5
#define SYSLED 4
#define WIFILED 3

hd44780_I2Cexp lcd; // declare lcd object: auto locate & config display for hd44780 chip

#define BLYNK_PRINT Serial

//La posizione degli elementi del'array notifiche relativi ai vari eventi
#define EVHUM 0
#define EVTEMP 1
#define EVMOV 2

int hum;
float temp;
int humLimit = 75;
int tempLimit = 25;
long int googleInterval = 300000L;

char auth[] = "cYc4mGATJA7eiiACUErh33-J6OMEYoKY";
const unsigned long delayNotificationMillis = 60000;
bool notificationAllowed[3] = {true, true, true};
bool systemDisabled = false;
virtuabotixRTC myRTC(7, 6, 5);

//Connessione con google spreadsheet
const char* host = "script.google.com";
const int httpsPort = 443;
String GAS_ID = "AKfycbyJgvc9Kg3UzkkN_IDy4rPSexJGnunSMjsVoP5gS6J2tvXId6MM";   // Google App Script id
WiFiSSLClient client;


WidgetTerminal terminal(V1);

#define DHTTYPE DHT11     // DHT 11
//#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321 <--- Tipo del lab
//#define DHTTYPE DHT21   // DHT 21, AM2301

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
/*
  char ssid[] = "***REMOVED***";
  char pass[] = "***REMOVED***";
*/

char ssid[] = "***REMOVED***";
char pass[] = "***REMOVED***";

BLYNK_CONNECTED() {
  // Request Blynk server to re-send latest values for all pins
  Blynk.syncAll();
}

void loop()
{
  Blynk.run();
  timer.run();
  myRTC.updateTime();
}

void sendSensor()
{
  terminal.flush(); //mi assicuro che il terminale non arrivi spezzettato

  readData();

  Blynk.virtualWrite(V5, hum);
  Blynk.virtualWrite(V6, temp);

  //IL SISTEMA VIENE DISABILITATO MA DOPO AVERE COMUNQUE PRESO TEMP E HUM
  if (systemDisabled == 1) {
    digitalWrite(SYSLED, LOW);
    digitalWrite(MOVLED, LOW);
    return; //Se il sistema è disabilitato, non fa nulla
  }
  digitalWrite(SYSLED, HIGH);

  //GESTISCO LE NOTIFICHE DEI SENSORI
  if (hum > humLimit) {
    if (notificationAllowed[EVHUM] == true) {
      notificationAllowed[EVHUM] = false; //Se ho appena lanciato una notifica, non notifico più se prima il valore non era sceso sotto il limite impostato
      String notifica = "L'umidità ha raggiunto valori troppo elevati: ";
      notifica += hum;
      notifica += "% alle:";
      notifica += printTime();
      //Blynk.email("Umidità", notifica); //Esempio di email
      Blynk.notify(notifica);
    }
  }
  else {
    notificationAllowed[EVHUM] = true; //Riattivo le notifiche se era sceso sotto la soglia
  }

  if (temp > tempLimit) {
    if (notificationAllowed[EVTEMP] == true) {
      notificationAllowed[EVTEMP] = false; //
      String notifica = "La temperatura ha raggiunto valori troppo elevati: ";
      notifica += temp;
      notifica += " C alle";
      notifica += printTime();
      //Blynk.email("Temperatura", notifica); //Esempio di email
      Blynk.notify(notifica);
    }
  }
  else {
    notificationAllowed[EVTEMP] = true;
  }

  if (digitalRead(IRPIN) == HIGH) {
    digitalWrite(MOVLED, HIGH);
    terminal.println("Movimento Rilevato");
    if (notificationAllowed[EVMOV] == true) {
      notificationAllowed[EVMOV] = false;
      String notifica = "Rilevato un movimento! Alle ";
      notifica += printTime();
      //Blynk.email("Temperatura", notifica); //Esempio di email
      Blynk.notify(notifica);
    }
  }
  else {
    digitalWrite(MOVLED, LOW);
    timer.setInterval(60000L, enableMovementNotification);
  }
}

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  lcd.begin(20, 4);
  lcd.print("Initializing...");
  // Set the current date, and time in the following format:
  // seconds, minutes, hours, day of the week, day of the month, month, year
  myRTC.setDS1302Time(00, 20, 12, 1, 14, 10, 2019);

  syncWidgets();

  pinMode(MOVLED, OUTPUT);
  pinMode(SYSLED, OUTPUT);
  dht.begin();

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.print("Please upgrade the firmware. Installed: ");
    Serial.println(fv);
  }

  //Mando i dati per la prima volta a google senza attendere il delay
  sendData();
  // Ogni secondo invia i sensori all'app
  timer.setInterval(1000L, sendSensor);
  //Ogni minuto invia i sensori a google
  timer.setInterval(googleInterval, sendData);
  //Ogni secondo stampa a terminale quali notifiche sono consentite e quali no
  timer.setInterval(5000L, debugSystem);
  //Mi assicuro che i widget abbiano gli stessi valori che ha arduino. Forse disabilitabile per risparmiare risorse
  timer.setInterval(1000L, syncWidgets);
  //Informazioni sulla rete ogni minuto
  timer.setInterval(60000L, printWifiData);
  timer.setInterval(60000L, printCurrentNet);
  //Aggiorna i dati sul display
  timer.setInterval(1000L, handleDisplay);
  //Controllo il led che indica connessione wifi
  timer.setInterval(1000L, checkWifi);
}


//Per il movimento è necessario usare timer per resettare le notifiche
void enableMovementNotification() {
  notificationAllowed[EVMOV] = true;
}

void readData() {
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  if (isnan(hum) || isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
}

// Function for Send data into Google Spreadsheet
void sendData()
{
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  readData();
  String string_temperature =  String(temp, 1);
  string_temperature.replace(".", ",");
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
  /*
  String line = client.readStringUntil('\n');
  Serial.println("reply was:");
  Serial.println("==========");
  //Serial.println(line);
  while (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
  Serial.println("=========="); */
  Serial.println("closing connection");
}

//Questa sezione mette sincronizza i valori nell'app con quelli nella memoria di arduino
void syncWidgets() {
  Blynk.virtualWrite(V3, tempLimit);
  Blynk.virtualWrite(V2, humLimit);
  Blynk.virtualWrite(V0, systemDisabled);
}

BLYNK_WRITE(V0)  {
  systemDisabled = param.asInt();
}

BLYNK_WRITE(V3)  {
  tempLimit = param.asFloat();
  terminal.print("Temp Limit: ");
  terminal.println(tempLimit);
}

BLYNK_WRITE(V2)  {
  humLimit = param.asFloat();
  terminal.print("Hum Limit: ");
  terminal.println(humLimit);
}

void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

String printTime() {
  String orario = "";
  orario += myRTC.hours;
  orario += ":";
  orario += myRTC.minutes;
  orario += ":";
  orario += myRTC.seconds;
  return orario;
}


String printDate() {
  String orario = "";
  orario += myRTC.dayofmonth;
  orario += "/";
  orario += myRTC.month;
  orario += "/";
  orario += myRTC.year;
  orario += myRTC.hours;
  orario += ".";
  orario += myRTC.minutes;
  orario += ".";
  orario += myRTC.seconds;
  return orario;
}

void handleDisplay() {
  lcd.clear();
  String time = "Time ";
  time += printTime();
  lcd.setCursor(0, 0);
  lcd.print(time);
  lcd.setCursor(0, 1);
  lcd.print("H: ");
  lcd.print(hum);
  lcd.print("% T: ");
  lcd.print(temp);
  lcd.print("C");
}

void checkWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WIFILED, LOW);
  } else {
    digitalWrite(WIFILED, HIGH);
  }
}

void debugSystem() {
  terminal.print("HumLimit:" );
  terminal.println(humLimit);
  terminal.print("TempLimit: ");
  terminal.println(tempLimit);
  for (int i = 0; i < 3; i++) {
    if (notificationAllowed[i]) {
      terminal.print(i);
      terminal.print(":allowed at ms ");
    }
    else {
      terminal.print(i);
      terminal.print(":NOT ALLOWED at ms ");
    }
    terminal.println(millis());
    terminal.flush();
  }
  if (systemDisabled == 1) {
    terminal.println("System Disabled");
  }
  else {
    terminal.println("Sistem Enabled");
  }
}
