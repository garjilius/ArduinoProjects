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

//Used to identify the events in the notification array
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
virtuabotixRTC myRTC(7, 6, 5); //Clock Pin Configuration


//Saving info used for recap email
//Position 0: Min - position 1: Max
int humStat[2] = {0, 0};
float tempStat[2] = {0, 0};
int numMov = 0;
int currentDay = 0;


//Google Sheets connection data
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

        //Read HTTP Characters
        if (readString.length() < 100) {
          readString += c;
          //Serial.print(c);
        }

        //If HTTP Request is successful
        if (c == '\n') {
          client.println("HTTP/1.1 200 OK");
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
          client.println("<a href=\"/?reset\"\">Delete Google Sheets Logs</a>");          //Reset Google Sheets log
          client.println("<a href=\"/?recovery\"\">Recovery</a><br/>");    //Start Recovery
          client.println("<br />");
          client.println("<br />");
          client.println("<a href=\"/?logNow\"\">Log Now!</a>");    //Log to both SD and Google Sheets
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
            logInterval = 60L * 1000L * minInterval; //Force value to Long
            timer.deleteTimer(timerGoogle);
            timer.deleteTimer(timerSD);
            timerGoogle = timer.setInterval(logInterval, sendData);
            timerSD = timer.setInterval(logInterval, logData);
            Serial.print("Log Interval set to: ");
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

  //Even if system is disabled, it stills sends sensor value to the app to update gauges
  if (systemDisabled == 1) {
    digitalWrite(SYSLED, LOW);
    return;
  }
  digitalWrite(SYSLED, HIGH);

  //Handle sensors' notifications
  if (hum > humLimit) {
    if (notificationAllowed[EVHUM] == true) {
      notificationAllowed[EVHUM] = false; //Disable notifications...
      String notifica = "Humidity is too high: ";
      notifica += hum;
      notifica += "% - :";
      notifica += printTime();
      //Blynk.email("Umidità", notifica); //Mail example
      Blynk.notify(notifica);
    }
  }
  else {
    notificationAllowed[EVHUM] = true; //Re-enable notifications if humidity went below threshold
  }

  if (temp > tempLimit) {
    if (notificationAllowed[EVTEMP] == true) {
      notificationAllowed[EVTEMP] = false; //
      String notifica = "Temperature is too high: ";
      notifica += temp;
      notifica += " C - ";
      notifica += printTime();
      Blynk.notify(notifica);
    }
  }
  else {
    notificationAllowed[EVTEMP] = true;
  }

  if (digitalRead(IRPIN) == HIGH) {
    terminal.println("Movement detected");
    if (notificationAllowed[EVMOV] == true) {
      notificationAllowed[EVMOV] = false;
      timer.setTimeout(60000L, enableMovementNotification); //Re-Enables movement notification after one minute
      String notifica = "Movement Detected - ";
      notifica += printTime();
      Blynk.notify(notifica);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  currentDay = myRTC.dayofmonth;
  pinMode(SYSLED, OUTPUT);
  pinMode(WIFILED, OUTPUT);
  dht.begin();
  Blynk.begin(auth, ssid, pass);
  server.begin();   // start the web server on port 80
  terminal.clear(); //Clear blynk terminal
  //Inizializzo il Display
  lcd.begin(20, 4);
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  //Inizializzo la SD
  lcd.setCursor(7, 2);
  if (!SD.begin(chipSelect)) {
    Serial.println("SD Card failed, or not present");
    lcd.print("- SD Err");
  } else {
    Serial.println("SD Card initialized.");
    lcd.print(" - SD OK");
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

  //First logging happens 30s after boot, regardless of logging interval settings
  timer.setTimeout(30000, logData);
  timer.setTimeout(30000, sendData);

  //Sets run frequency for used functions
  timer.setInterval(1000L, sendSensor);
  timerGoogle = timer.setInterval(logInterval, sendData);
  timerSD = timer.setInterval(logInterval, logData);
  timer.setInterval(15000L, handleDisplay);
  timer.setInterval(5000L, checkWifi);
  timer.setInterval(1800000L, handleReports);
}


//Re Enables movement notifications
void enableMovementNotification() {
  notificationAllowed[EVMOV] = true;
}

//Reads data from DHT sensor
void readData() {
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  if (isnan(hum) || isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  manageStats(temp, hum);
}

//Logs data do Google Sheets
void sendData() {
  lcd.setCursor(4, 3);
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No Connection");
    if (needRecovery != 1) {
      needRecovery = 1;
      //Writing the 'needRecovery' value to Arduino's EEPROM allows me to retrieve it even after rebooting
      EEPROM.write(0, needRecovery);
    }
    return;
  }
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    lcd.print("CLOUD ERR");
    if (needRecovery != 1) {
      needRecovery = 1;
      EEPROM.write(0, needRecovery);
    }
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
      lcd.print("CLOUD OK");
      break;
    }
  }
}

//Logs data to SD
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
  lcd.setCursor(14, 3);
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.print("LoggedToSD: ");
    Serial.println(dataString);
    lcd.print("SD OK");
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening log file");
    lcd.print("SD ERR");
  }
}


/*
   This functions allows me to upload to Google Sheets data that had only been logged to the SD card due to a lack of connection
*/
void recovery() {
  File myFile;
  lcdClearLine(3);
  myFile = SD.open("LOG.TXT");
  if (myFile) {
    Serial.println("Recovery File Opened");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      String date = myFile.readStringUntil(' ');
      date.replace("\n", ""); //Removes newlines
      date.replace("\r", "");
      String temp = myFile.readStringUntil(' ');
      String hum = myFile.readStringUntil(' ');
      String needRecovery = myFile.readStringUntil('\r');
      int toRecover = needRecovery.toInt();

      if (toRecover == 1) {
        if (!client.connect(host, httpsPort)) {
          Serial.println("Recovery failed");
          lcd.print("Recovery FAILED");
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
            lcd.print("Recovery SUCCESS");
            break;
          }
        }
      }
    }
    myFile.close();
    deleteSDLog();
    needRecovery = 0;

    //Writing the 'needRecovery' value to Arduino's EEPROM allows me to retrieve it even after rebooting
    EEPROM.write(0, needRecovery);
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening log file");
    lcd.print("Recovery FAILED");

  }
}

//Read from Blynk's server values to be re-synced to arduino when rebooted
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
  Serial.print("IP: ");
  Serial.println(ip);
}


String printTime() {
  String orario = "";
  orario += myRTC.hours;
  orario += ":";
  orario += myRTC.minutes;
  //orario += ":";
  //orario += myRTC.seconds;
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
  lcd.setCursor(0, 0);
  lcd.print(printTime());
  lcd.print(" H:");
  lcd.print(hum);
  lcd.print("% T:");
  lcd.print(temp);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("IP: ");
  lcd.print(WiFi.localIP());
  lcdClearLine(3);
  lcd.print("Log:");
}

void checkWifi() {
  lcd.setCursor(0, 2);
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WIFILED, LOW);
    lcd.print("WiFi ERR");
  } else {
    digitalWrite(WIFILED, HIGH);
    lcd.print("WiFi OK");
  }
}

//Check if log lines need to be synced from the SD card to google sheets
void recoveryManager() {
  if ((needRecovery == 1) && (WiFi.status() == WL_CONNECTED)) {
    recovery();
  }
  else {
    lcdClearLine(3);
    lcd.print("NO RECOVERY NEEDED");
    Serial.println("NO CONNECTION OR NO RECOVERY NEEDED!");
  }
}

//Delete all lines in Google Sheets Log
void resetSheets() {
  lcdClearLine(3);
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    lcd.print("CLOUD RESET OK");
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
      lcd.print("CLOUD RESET OK");
      break;
    }
  }
}

//Delete all lines in SD Log
void deleteSDLog() {
  lcdClearLine(3);
  bool fileRemoved = SD.remove("LOG.TXT");
  if (fileRemoved) {
    lcd.print("SD RESET OK");
    Serial.println("Log/Recovery file succesfully removed");
  } else {
    lcd.print("SD RESET ERR");
    Serial.println("Failed deleting log file");
  }
}

//Clears line 'i' and moves the cursor back to the start of that line
void lcdClearLine(int i) {
  lcd.setCursor(0, i);
  lcd.print("                    ");
  lcd.setCursor(0, i);
}

/*
 * Functions to manage stats and send reports have been split into separate functions to allow more flexibility:
 * For example, you can send a mail recap if the date has changed (= on a new day), but you can also send a recap
 * explicitly requesting it via control panel and so on
 */


//Keeps min and max temperature updated
void manageStats(float temp, int hum) {
  if (temp < tempStat[0]) {
    tempStat[0] = temp;
  }
  if (temp > tempStat[1]) {
    tempStat[1] = temp;
  }
  if (hum < humStat[0]) {
    tempStat[0] = temp;
  }
  if (hum > humStat[1]) {
    tempStat[1] = temp;
  }
}

//Resets all stats
void resetStats() {
  tempStat[0] = 0;
  tempStat[1] = 1;
  humStat[0] = 0;
  humStat[1] = 1;
  numMov = 0;
}

//If the date has changed, returns true and uptates currentDay
bool dateChanged() {
  //If date changed
  if (currentDay != myRTC.dayofmonth) {
    currentDay = myRTC.dayofmonth;
    return true;
  }
  return false;
}

//Sends the report mail using Blynk
void sendReport() {
  String report = "Min temp of the day: ";
  report += tempStat[0];
  report += " C, max temp of the day: ";
  report += tempStat[1];
  report += "C \n min hum of the day: ";
  report += humStat[0];
  report += " %, max hum of the day: ";
  report += humStat[1];
  report += "% \n Number of movements detected: ";
  report += numMov;

  //After sending the email, stats get reset
  Blynk.email(MY_EMAIL, "Your daily report", report);
  resetStats();
}

/*checks if the data has changed and if it has, sends a report.
It's handy having a separate function to do it because it can be called repeatedly in a timer
*/
void handleReports() {
  if (dateChanged())
    sendReport();
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
