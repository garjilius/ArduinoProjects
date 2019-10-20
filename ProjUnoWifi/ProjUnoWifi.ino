#include <DHT.h>
#include <SPI.h>
#include <BlynkSimpleWiFiNINA.h>
#include <virtuabotixRTC.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>
#include "arduino_secrets.h"


//#define BLYNK_PRINT Serial
#define DHTPIN 2
#define IRPIN 9
#define SYSLED 4
#define WIFILED 3

hd44780_I2Cexp lcd;

//La posizione degli elementi del'array notifiche relativi ai vari eventi
#define EVHUM 0
#define EVTEMP 1
#define EVMOV 2

#define DHTTYPE DHT11     // DHT 11
//#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321 <--- Tipo del lab

WiFiSSLClient client;
WiFiServer server(80);

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
WidgetTerminal terminal(V1);

int status = WL_IDLE_STATUS;
String readString;
const int chipSelect = 8;
int hum;
float temp;
int humLimit = 75;
int tempLimit = 25;
int timerGoogle;
int timerSD;
long int logInterval = 600000L;
byte needRecovery = 0;
//Token Blynk
char auth[] = "cYc4mGATJA7eiiACUErh33-J6OMEYoKY";
bool notificationAllowed[3] = {true, true, true};
bool systemDisabled = false;
virtuabotixRTC myRTC(7, 6, 5); //Configurazione pin orologio

//Connessione con google sheets
const char* host = "script.google.com";
const int httpsPort = 443;

String GAS_ID = "AKfycbyJgvc9Kg3UzkkN_IDy4rPSexJGnunSMjsVoP5gS6J2tvXId6MM";   // Google App Script id

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

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
          client.println("<link rel='stylesheet' type='text/css' href='https://dl.dropbox.com/s/oe9jvh9pmyo8bek/styles.css?dl=0' />");
          client.println("<TITLE>Arduino Control Panel</TITLE>");
          client.println("</HEAD>");
          client.println("<BODY>");
          client.println("<H1>Arduino Control Panel</H1>");
          client.println("<hr />");
          client.println("<H2>Welcome, Emanuele</H2>");
          client.println("<br />");
          client.println("<img src=\"https://dl.dropbox.com/s/xuj9q90zsbdyl2n/LogoUnisa.png?dl=0\" alt=\"Circuito\" style=\"width:200px;height:200px;\">");
          client.println("<br />");
          client.println("<br />");
          client.println("<br />");
          client.println("<a href=\"/?deleteSD\"\">Delete SD Logs</a>");
          client.println("<a href=\"/?reset\"\">Delete Google Sheets Logs</a>");          //Resetta i log so google sheets
          client.println("<a href=\"/?recovery\"\">Recovery</a><br/>");    //Link che avvia la modalità recovery
          client.println("<br />");
          client.println("<br />");
          client.println("<a href=\"/?logNow\"\">Log Now!</a>");    //Salva temperatura e umidità attuali su SD e google spreadsheets
          client.println("<a href=\"/?\"\">Reload Page</a><br/>");
          client.println("<br />");
          client.println("<br />");
          client.println("<form action="">");
          client.println("Frequenza Logging (minuti)");
          int minInterval = logInterval / 60;
          minInterval = minInterval / 1000;
          String interval = "<input type=\"number\" name=\"logInterval\" min=\"1\" max=\"1440\" value=";
          interval += minInterval;
          interval += ">";
          client.println(interval);
          client.println("<input type=\"submit\">");
          client.println("</form>");
          client.println("<br />");
          client.println("<b>Delete SD Logs:</b> deletes the log file from the SD Card");
          client.println("<br />");
          client.println("<b>Delete Google Sheets Logs:</b> deletes the log from Google Sheets");
          client.println("<br />");
          client.println("<b>Recovery:</b> syncs to google sheets data that has been logged when offline");
          client.println("<br />");
          client.println("<br />");
          client.println("</BODY>");
          client.println("</HTML>");

          client.stop();
          //Passo i comandi ad Arduino in get, insieme all'URL
          if (readString.indexOf("?recovery") > 0) {
            recoveryManager();
          }
          if (readString.indexOf("?logNow") > 0) {
            logData();
            sendData();
          }
          if (readString.indexOf("?reset") > 0) {
            resetSheets();
          }
          if (readString.indexOf("?deleteSD") > 0) {
            deleteSDLog();
          }
          if (readString.indexOf("?logInterval") > 0) {
            int startIndex = readString.indexOf("=") + 1 ;
            int endIndex = readString.indexOf("HTTP") - 1;
            int minInterval = readString.substring(startIndex, endIndex).toInt();
            logInterval = 60L * 1000L * minInterval; //Forzo il valore a diventare un long
            timer.deleteTimer(timerGoogle);
            timer.deleteTimer(timerSD);
            timerGoogle = timer.setInterval(logInterval, sendData);
            timerSD = timer.setInterval(logInterval, logData);
            Serial.print("Intervallo di aggiornamento settato a: ");
            Serial.println(logInterval);
          }
          readString = "";
        }
      }
    }
  }
}

void sendSensor() {
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
      notificationAllowed[EVHUM] = false; //Se ho appena lanciato una notifica, non notifico più se prima il valore non era ri-sceso sotto il limite impostato
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
      timer.setTimeout(60000L, enableMovementNotification); //Riattivo le notifiche un minuto dopo averle disattivate
      String notifica = "Rilevato un movimento! Alle ";
      notifica += printTime();
      Blynk.notify(notifica);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(SYSLED, OUTPUT);
  pinMode(WIFILED, OUTPUT);
  dht.begin();
  Blynk.begin(auth, ssid, pass);
  server.begin();   // start the web server on port 80
  terminal.clear(); //Svuoto il terminale blynk
  //Inizializzo il Display
  lcd.begin(20, 4);
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  lcd.setCursor(0, 2);
  //Inizializzo la SD
  if (!SD.begin(chipSelect)) {
    Serial.println("SD Card failed, or not present");
    lcd.print("SD Card Error");
  } else {
    Serial.println("card initialized.");
    lcd.print("SD Card OK");
  }
  // Set the current date, and time in the following format:
  // seconds, minutes, hours, day of the week, day of the month, month, year
  myRTC.setDS1302Time(00, 20, 12, 1, 14, 10, 2019);

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

  //Il primo logging fiene fatto a 30s dall'avvio, indipendentemente dal log Interval
  timer.setTimeout(30000, logData);
  timer.setTimeout(30000, sendData);

  //Invia i sensori all'app
  timer.setInterval(1000L, sendSensor);
  //Loggo i dati su Google Sheets ogni logInterval ms
  timerGoogle = timer.setInterval(logInterval, sendData);
  //Loggo i dati su sd ogni logInterval ms
  timerSD = timer.setInterval(logInterval, logData);

  //Informazioni sulla rete ogni minuto
  //timer.setInterval(60000L, printWifiData);
  //Aggiorna i dati sul display
  timer.setInterval(1000L, handleDisplay);
  //Controllo il led che indica connessione wifi
  timer.setInterval(5000L, checkWifi);

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

//Log dei dati su Google Sheets
void sendData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No Connection");
    if (needRecovery != 1) {
      needRecovery = 1;
      //Scrivo anche sulla memoria permanente se c'è necessità o meno di recovery, in modo da poterlo leggere anche al riavvio di arduino
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
  String url = "/macros/s/" + GAS_ID + "/exec?temperature=" + temp + "&humidity=" + hum;

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("Logged to Google Sheets");
      break;
    }
  }
}

//Log dei dati su SD
void logData() {
  String dataString;
  //dataString += printDate(); //Andrà riattivato quando connetterò l'orologio
  dataString += "19/10/2019-22.10.00";
  dataString += " ";
  dataString += temp;
  dataString += " ";
  dataString += hum;
  dataString += " ";
  dataString += needRecovery;

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


/*
   Questa funzione serve a caricare su Google i dati che sono stati loggati solo su SD per assenza di connessione
*/
void recovery() {
  //ATTENZIONE! QUANDO FACCIO IL RECOVERY DEVO POI RICORDARMI DI PASSARGLI LA DATA ORIGINALE!
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

    //Scrivo anche sulla memoria permanente se c'è necessità o meno di recovery, in modo da poterlo leggere anche al riavvio di arduino
    EEPROM.write(0, needRecovery);
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening log file");
  }
}

//LEGGO DAL SERVER BLYNK ALCUNI VALORI DI IMPOSTAZIONI NECESSARI PER ARDUINO
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
  } else {
    digitalWrite(WIFILED, HIGH);
  }
}

//Controlla la necessità di recovery ed eventualmente avvia il processo
void recoveryManager() {
  if ((needRecovery == 1) && (WiFi.status() == WL_CONNECTED)) {
    recovery();
  }
  else {
    Serial.println("NO CONNECTION OR NO RECOVERY NEEDED!");
  }
}

//Cancella tutte le righe dal file log su Google Sheets
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

//Cancella il file di log dalla SD
void deleteSDLog() {
  bool fileRemoved = SD.remove("LOG.TXT");
  if (fileRemoved) {
    Serial.println("Log/Recovery file succesfully removed");
  } else {
    Serial.println("Failed deleting log file");
  }
}

/*
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
*/
