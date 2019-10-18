#include <DHT.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <BlynkSimpleWiFiNINA.h>
#include <virtuabotixRTC.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header //
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>

#define DHTPIN 2
#define IRPIN 9
#define SYSLED 4
#define WIFILED 3

hd44780_I2Cexp lcd; // declare lcd object: auto locate & config display for hd44780 chip

#define BLYNK_PRINT Serial

int status = WL_IDLE_STATUS;
WiFiServer server(80);
String readString;

//La posizione degli elementi del'array notifiche relativi ai vari eventi
#define EVHUM 0
#define EVTEMP 1
#define EVMOV 2
const int chipSelect = 8;

int hum;
float temp;
int humLimit = 75;
int tempLimit = 25;
long int logInterval = 60000L;
byte needRecovery = 0;

char auth[] = "cYc4mGATJA7eiiACUErh33-J6OMEYoKY";
bool notificationAllowed[3] = {true, true, true};
bool systemDisabled = false;
virtuabotixRTC myRTC(7, 6, 5); //Configurazione pin orologio

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

//char ssid[] = "***REMOVED***";
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

  //SERVER!
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        //Leggo i caratteri da HTTP
        if (readString.length() < 100) {
          //Inserisco i caratteri nella stringa
          readString += c;
          //Serial.print(c);
        }

        //Se la richiesta HTTP è andata a buon fine
        if (c == '\n') {
          //Serial.println(readString); //scrivi sul monitor seriale per debugging

          client.println("HTTP/1.1 200 OK"); //Invio nuova pagina
          client.println("Content-Type: text/html");
          client.println();
          client.println("<HTML>");
          client.println("<HEAD>");
          client.println("<meta name='apple-mobile-web-app-capable' content='yes' />");
          client.println("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");
          client.println("<link rel='stylesheet' type='text/css' href='http://www.progettiarduino.com/uploads/8/1/0/8/81088074/style3.css' />");
          client.println("<TITLE>Arduino Control Panel</TITLE>");
          client.println("</HEAD>");
          client.println("<BODY>");
          client.println("<H1>Arduino Control Panel</H1>");
          client.println("<hr />");
          client.println("<br />");
          client.println("<H2>Welcome, Emanuele</H2>");
          client.println("<br />");
          client.println("<a href=\"/?deleteSD\"\">Delete SD Logs</a>");
          client.println("<a href=\"/?reset\"\">Delete Google Sheets Logs</a>");          //Resetta i log so google sheets
          client.println("<a href=\"/?recovery\"\">Recovery</a><br />");    //Link che avvia la modalità recovery
          client.println("<br />");
          client.println("Delete SD Logs: deletes the log file from the SD Card");
          client.println("<br />");
          client.println("Delete Google Sheets Logs: deletes the log from Google Sheets");
          client.println("<br />");
          client.println("Recovery: syncs to google sheets data that has been logged when offline");
          client.println("<br />");

          client.println("<form action="">");
          client.println("Frequenza Logging (minuti)");
          int minInterval = logInterval / 60;
          minInterval = minInterval / 1000;
          String interval = "<input type=\"number\" name=\"logInterval\" min=\"1\" max=\"1440\" value=";
          interval += minInterval;
          interval += ">";
          //client.println("<input type=\"number\" name=\"logInterval\" min=\"1\" max=\"1440\" value=10>");
          client.println(interval);
          client.println("<input type=\"submit\">");
          client.println("</form>");

          client.println("<br />");
          client.println("<br />");

          client.println("</BODY>");
          client.println("</HTML>");

          delay(1);
          client.stop();
          //Controlli su Arduino: Se è stato premuto un pulsante sul webserver
          if (readString.indexOf("?recovery") > 0) {
            recoveryManager();
          }
          if (readString.indexOf("?reset") > 0) {
            resetSheets();
          }
          if (readString.indexOf("?deleteSD") > 0) {
            Serial.println("Il reset di google fogli ha problemi, funzione disabilitata");
            deleteSDLog();
          }
          if (readString.indexOf("?logInterval") > 0) {
            int startIndex = readString.indexOf("=") + 1 ;
            int endIndex = readString.indexOf("HTTP") - 1;
            String intervalString = readString.substring(startIndex, endIndex); //Devo estrarre il valore intero che indica l'intervallo
            //Serial.println(intervalString);
            int minInterval = intervalString.toInt();
            Serial.println(minInterval);
            logInterval = 60L * 1000L * intervalString.toInt(); //Forzo il valore a diventare un long
            Serial.print("Intervallo di aggiornamento settato a: ");
            Serial.println(logInterval);
          }
          //Cancella la stringa una volta letta
          readString = "";
        }
      }
    }
  }
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
    timer.setInterval(60000L, enableMovementNotification); //attenzione, così facendo la abilita ogni tot secondi indipendentemente dal movimento
  }
}

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  server.begin();                           // start the web server on port 80
  lcd.begin(20, 4);
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  lcd.setCursor(0, 2);
  if (!SD.begin(chipSelect)) {
    Serial.println("SD Card failed, or not present");
    lcd.print("SDCard Error");
  } else {
    Serial.println("card initialized.");
    lcd.print("SD Card OK");
  }
  // Set the current date, and time in the following format:
  // seconds, minutes, hours, day of the week, day of the month, month, year
  myRTC.setDS1302Time(00, 20, 12, 1, 14, 10, 2019);

  syncWidgets();

  pinMode(SYSLED, OUTPUT);
  pinMode(WIFILED, OUTPUT);
  dht.begin();

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.print("Please upgrade the firmware. Installed: ");
    Serial.println(fv);
  }

  EEPROM.get(0, needRecovery);
  if ((needRecovery == 1) && (WiFi.status() == WL_CONNECTED)) {
    Serial.println("Lines need recovery, Network Available");
  }

  printWifiData();
  // Ogni secondo invia i sensori all'app
  timer.setInterval(1000L, sendSensor);
  //Ogni minuto invia i sensori a google
  timer.setInterval(logInterval, sendData);
  //Ogni secondo stampa a terminale quali notifiche sono consentite e quali no
  timer.setInterval(300000L, debugSystem);
  //Mi assicuro che i widget abbiano gli stessi valori che ha arduino. Forse disabilitabile per risparmiare risorse
  timer.setInterval(1000L, syncWidgets);
  //Informazioni sulla rete ogni minuto
  //timer.setInterval(60000L, printWifiData);
  //timer.setInterval(60000L, printCurrentNet);
  //Aggiorna i dati sul display
  timer.setInterval(1000L, handleDisplay);
  //Controllo il led che indica connessione wifi
  timer.setInterval(5000L, checkWifi);
  //Loggo i dati su sd ogni tot tempo
  timer.setInterval(logInterval, logData);
  //Eseguo il recovery manager periodicamente
  //timer.setInterval(120000, recoveryManager);
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
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No Connection");
    if (needRecovery != 1) {
      needRecovery = 1;
      EEPROM.write(0, needRecovery);
    }
    return;
  }
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
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
      Serial.println("Logged to Google Sheets");
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
    Serial.println("=========="); 
  Serial.println("closing connection"); */
}

void logData() {
  String string_temperature =  String(temp, 1);
  string_temperature.replace(".", ",");
  String string_humidity =  String(hum, DEC);
  String dataString;
  //dataString += printDate();
  dataString += "17/10/2019-21.17.16";
  dataString += " ";
  dataString += string_temperature;
  dataString += " ";
  dataString += string_humidity;
  dataString += " ";
  dataString += needRecovery;

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("log.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.print("LoggedToSD: ");
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening log file");
  }
}

//L'idea di questa funzione è che se c'era stato un errore di connessione ma poi la connessione torna,
//allora vengono caricati su google i dati che erano mancanti
void recovery() {
  //ATTENZIONE! QUANDO FACCIO IL RECOVERY DEVO POI RICORDARMI DI PASSARGLI LA DATA ORIGINALE!
  Serial.println("Entering Recovery...");
  File myFile;
  myFile = SD.open("LOG.TXT");
  if (myFile) {
    Serial.println("Recovery File Opened");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      String date = myFile.readStringUntil(' ');
      date.replace("\n", ""); //Per qualche motivo sembrano esserci degli a capo
      date.replace("\r", "");
      String temp = myFile.readStringUntil(' ');
      String hum = myFile.readStringUntil(' ');
      String needRecovery = myFile.readStringUntil('\r');
      int toRecover = needRecovery.toInt();
      /*Serial.print("Date: ");
        Serial.println(date);
        Serial.print("Hour: ");
        Serial.print("Temp: ");
        Serial.println(temp);
        Serial.print("Hum: ");
        Serial.println(hum);
        Serial.print("ToRecover: ");
        Serial.println(toRecover);*/
      if (toRecover == 1) {
        if (!client.connect(host, httpsPort)) {
          Serial.println("Recovery failed");
          return;
        }
        Serial.println("Recovering Line...");
        String url = "/macros/s/" + GAS_ID + "/exec?temperature=" + temp + "&humidity=" + hum + "&date=" + date;
        Serial.print("requesting URL: ");
        Serial.println(url);

        client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "User-Agent: BuildFailureDetectorESP8266\r\n" +
                     "Connection: close\r\n\r\n");

        Serial.println("request sent");
        while (client.available()) {
          char c = client.read();
          Serial.print(c);
        }
        while (client.connected()) {
          String line = client.readStringUntil('\n');
          if (line == "\r") {
            Serial.println("Line recovered");
            break;
          }
        }
      }
    }
    myFile.close();
    deleteSDLog();
    needRecovery = 0;

    EEPROM.write(0, needRecovery);
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening log file");
  }
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
  orario += "-";
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
    //Blynk.begin(auth, ssid, pass); Riconnessione, ma è bloccante: Se non c'è rete si ferma tutto
  } else {
    digitalWrite(WIFILED, HIGH);
  }
}

void debugSystem() {

  Serial.print("NeedRecovery?: ");
  Serial.println(needRecovery);
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

void recoveryManager() {
  if ((needRecovery == 1) && (WiFi.status() == WL_CONNECTED)) {
    recovery();
  }
  else {
    Serial.println("NO CONNECTION OR NO RECOVERY NEEDED!");
  }
}


void resetSheets() {
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }
  String url = "/macros/s/" + GAS_ID + "/exec?reset=1";

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("Reset request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("Google Sheets Reset: SUCCESS");
      break;
    }
  }
}

void deleteSDLog() {
  bool fileRemoved = SD.remove("LOG.TXT");
  if (fileRemoved) {
    Serial.println("Log/Recovery file succesfully removed");
  } else {
    Serial.println("Failed deleting log file");
  }
}
