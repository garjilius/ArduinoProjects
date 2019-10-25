#include <DHT.h>
#include <BlynkSimpleWiFiNINA.h>
#include <virtuabotixRTC.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>
#include "arduino_secrets.h"

#define DHTPIN 2
#define IRPIN 9
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

String readString;
const int chipSelect = 8;
int hum;
float temp;
int humLimit = 75;
int tempLimit = 25;
int timerGoogle;
bool sdOK = false;
long int logInterval = 600000L;
byte needRecovery = 0;
//Token Blynk
char auth[] = "QGJM5LWaUibrTKblRJ-EGO3dllEngTD1";
bool notificationAllowed[3] = {true, true, true};
bool systemDisabled = false;
virtuabotixRTC myRTC(7, 6, 5); //Clock Pin Configuration

//Saving info used for recap email
//Position 0: Min - position 1: Max
int humStat[2] = {100, -100};
float tempStat[2] = {100, -100};
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

void loop() {
  Blynk.run();
  timer.run();
  myRTC.updateTime();

  //Retries to open SD if failed
  if (!sdOK) {
    if (sdOK = SD.begin(chipSelect)) {
      Serial.println(F("SD INITIALIZED"));
    }
    else {
      Serial.println(F("SD NOT WORKING!"));
    }
  }

  //SERVER!
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {
    //Serial.println("new client");
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        //Read HTTP Characters
        if (readString.length() < 100) {
          readString += c;
        }
        //If HTTP Request is successful
        if (c == '\n') {
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/html"));
          client.println(F("Connection: close"));  // the connection will be closed after completion of the response
          //client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println(F("<!DOCTYPE HTML>"));
          client.println(F("<HTML>"));
          client.println(F("<HEAD>"));
          client.println(F("<link rel='stylesheet' type='text/css' href='https://dl.dropbox.com/s/oe9jvh9pmyo8bek/styles.css?dl=0'/>"));
          //Most of the page gets added via remote javascript to save space on arduino and speed things up
          client.println(F("<script type=\"text/javascript\" src=\"https://dl.dropbox.com/s/cmtov3p8tj29wbs/jsextra.js?dl=0\"></script>"));
          client.println(F("<TITLE>Arduino Control Panel</TITLE>"));
          client.println(F("</HEAD>"));
          client.println(F("<BODY>"));
          client.println(F("<div id='mainBody'></div>"));
          client.println(F("<form action="">"));
          client.println(F("Frequenza Logging (minuti)"));
          int minInterval = logInterval / 60;
          minInterval = minInterval / 1000;
          String interval = "<input type=\"number\" name=\"logInterval\" min=\"1\" max=\"1440\" value=";
          interval += minInterval;
          interval += ">";
          client.println(interval);
          client.println(F("<input type=\"submit\" class=\"button button1\">"));
          client.println(F("<div id='leg'></div>"));
          client.println(F("</BODY>"));
          client.println(F("</HTML>"));
          // give the web browser time to receive the data
          delay(50);
          client.stop();
          //Serial.println("client disonnected");
          //E' stata settata una data
          if (readString.indexOf("?date") > 0) {
            //giorno-mese-anno-ora-minuto-secondo
            int date[7];
            date[6] = readString.substring(11, 13).toInt(); //dayofweek
            date[0] = readString.substring(14, 16).toInt(); //giorno
            date[1] = readString.substring(17, 19).toInt(); //mese
            date[2] = readString.substring(20, 24).toInt(); //anno
            date[3] = readString.substring(25, 27).toInt(); //ora
            date[4] = readString.substring(28, 30).toInt(); //minuto
            date[5] = readString.substring(31, 33).toInt(); //secondo
            // seconds, minutes, hours, day of the week, day of the month, month, year
            myRTC.setDS1302Time(date[5], date[4], date[3], date[6], date[0], date[1], date[2]);
            currentDay = myRTC.dayofmonth;
            Serial.println(F("Time Set"));
          }
          if (readString.indexOf("?recovery") > 0) {
            recoveryManager();
          }
          if (readString.indexOf("?logNow") > 0) {
            sendData();
          }
          if (readString.indexOf("?reset") > 0) {
            resetSheets();
          }
          if (readString.indexOf("?deleteSD") > 0) {
            deleteSDLog();
          }
          if (readString.indexOf("?sendReport") > 0) {
            Serial.println("Send report...");
            sendReport();
          }
          if (readString.indexOf("?logInterval") > 0) {
            //Getting the useful data between startindex and endindex using indexOf
            int minInterval = readString.substring(readString.indexOf("=") + 1, readString.indexOf("HTTP") - 1).toInt();
            logInterval = 60L * 1000L * minInterval; //Force value to Long
            timer.deleteTimer(timerGoogle);
            timerGoogle = timer.setInterval(logInterval, sendData);
            Serial.print(F("Log Interval set to: "));
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
    return;
  }

  //Handle sensors' notifications
  if (hum > humLimit) {
    if (notificationAllowed[EVHUM] == true) {
      notificationAllowed[EVHUM] = false; //Disable notifications...
      String notifica = "Humidity is too high: ";
      notifica += hum;
      notifica += "% - ";
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

  if (digitalRead(IRPIN) == HIGH && notificationAllowed[EVMOV] == true) {
    notificationAllowed[EVMOV] = false;
    timer.setTimeout(60000L, enableMovementNotification); //Re-Enables movement notification after one minute
    numMov++;
    String notifica = "Movement Detected - ";
    notifica += printTime();
    Blynk.notify(notifica);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(SYSLED, OUTPUT);
  pinMode(WIFILED, OUTPUT);
  pinMode(DHTPIN, INPUT);
  pinMode(IRPIN, INPUT);
  WiFi.setTimeout(60000);
  dht.begin();
  server.begin();   // start the web server on port 80
  //Display Initialization
  lcd.begin(20, 4);
  lcd.setCursor(7, 2);
  //SD Initialization
  sdOK = SD.begin(chipSelect);
  if (!sdOK) {
    //Commented out as it will be printed in the loop function
    //Serial.println(F("SD Card failed"));
    lcd.print(F("- SD Err"));
  } else {
    Serial.println(F("SD Card initialized."));
    lcd.print(F(" - SD OK"));
  }

  WiFi.begin(ssid, pass);
  Blynk.config(auth);

  // Set the current date, and time in the following format:
  // seconds, minutes, hours, day of the week, day of the month, month, year
  myRTC.setDS1302Time(00, 00, 19, 5, 25, 10, 2019);
  currentDay = myRTC.dayofmonth;

  //Reads from eeprom if there's need for recovery or not
  EEPROM.get(0, needRecovery);
  if ((needRecovery == 1) && (WiFi.status() == WL_CONNECTED)) {
    Serial.println(F("Need recovery"));
  }

  printWifiData();

  /*
    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.print(F("Please upgrade the firmware. Installed: "));
    Serial.println(fv);
    }
  */

  //First logging happens 30s after boot, regardless of logging interval settings
  timer.setTimeout(30000, sendData);

  //Sets run frequency for used functions
  timer.setInterval(3000, sendSensor);
  timerGoogle = timer.setInterval(logInterval, sendData);
  timer.setInterval(15000, handleDisplay);
  timer.setInterval(60000, checkWifi);
  timer.setInterval(1800000L, handleReports);
  //timer.setInterval(5000, debugSystem);
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
    Serial.println(F("DHT Read Fail"));
    return;
  }
  manageStats(temp, hum);
}

//Logs data do Google Sheets
void sendData() {
  lcd.setCursor(4, 3);
  if (!client.connect(host, httpsPort)) {
    Serial.println(F("Connection failed"));
    lcd.print(F("CLOUD ERR"));
    //Log data su SD IF AND ONLY IF logging to google has failed, to save space on microsd and computing power
    logData();
    if (needRecovery != 1) {
      needRecovery = 1;
      EEPROM.write(0, needRecovery);
    }
    return;
  }
  readData();
  String url = "/macros/s/" + GAS_ID + "/exec?temp=" + temp + "&hum=" + hum;

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println(F("Logged to Google Sheets"));
      lcd.print(F("CLOUD OK"));
      break;
    }
  }
}

//Logs data to SD
void logData() {
  String dataString;
  dataString += printDate(); //Andrà riattivato quando connetterò l'orologio
  dataString += " ";
  dataString += temp;
  dataString += " ";
  dataString += hum;

  File dataFile = SD.open("log.txt", FILE_WRITE);
  //30000 bytes are approx 1000 lines
  if(dataFile.size() > 30000) {
    deleteSDLog();
    dataFile = SD.open("log.txt", FILE_WRITE);
  }
  lcd.setCursor(13, 3);
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.print(F("LoggedToSD: "));
    Serial.println(dataString);
    lcd.print(" SD OK");
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println(F("error opening log file"));
    lcd.print(F("SD ERR"));
    sdOK = false;
  }
}


/*
   This functions allows me to upload to Google Sheets data that had only been logged to the SD card due to a lack of connection
*/
void recovery() {
  File myFile = SD.open("LOG.TXT");
  if (myFile) {
    while (myFile.available()) {
      String dateS = myFile.readStringUntil(' ');
      String tempS = myFile.readStringUntil(' ');
      String humS = myFile.readStringUntil('\n');

      if (!client.connect(host, httpsPort)) {
        Serial.println(F("Recovery failed"));
        lcdClearLine(3);
        lcd.print(F("Recovery FAILED"));
        return;
      }
      String url = "/macros/s/" + GAS_ID + "/exec?temp=" + tempS + "&hum=" + humS + "&date=" + dateS;
      url.replace("\n", ""); //Removes newlines
      url.replace("\r", "");
      Serial.println(F(url));

      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Connection: close\r\n\r\n");

      while (client.connected()) {
        String line = client.readStringUntil('\n');
        //Serial.println(line);
        if (line == "\r") {
          Serial.println(F("Line recovered"));
          lcdClearLine(3);
          lcd.print(F("Recovery SUCCESS"));
          break;
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
    Serial.println(F("error opening log file"));
    lcd.print(F("Recovery FAILED"));
    sdOK = false;
  }
}

//Read from Blynk's server values to be re-synced to arduino when rebooted
BLYNK_WRITE(V0)  {
  systemDisabled = param.asInt();
}

BLYNK_WRITE(V3)  {
  tempLimit = param.asFloat();
}

BLYNK_WRITE(V2)  {
  humLimit = param.asFloat();
}

// print your board's IP address:
void printWifiData() {
  Serial.println(WiFi.localIP());
}


String printTime() {
  String orario = "";
  orario += myRTC.hours;
  orario += ":";
  orario += myRTC.minutes;
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

//Handles info on the i2c display
void handleDisplay() {
  lcd.setCursor(0, 0);
  lcd.print(printTime());
  lcd.print(" H:");
  lcd.print(hum);
  lcd.print("% T:");
  lcd.print(temp);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print(F("IP: "));
  lcd.print(WiFi.localIP());
  lcd.setCursor(7, 2);
  if (!sdOK) {
    lcd.print(F(" - SD Err"));
  } else {
    lcd.print(F(" - SD OK"));
  }
  lcd.setCursor(0, 3);
  lcd.print(F("Log:"));
}

//If WIFI is not working, wifi led is switched off and we attempt to reconnect to wifi and to blynk servers
void checkWifi() {
  lcd.setCursor(0, 2);
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WIFILED, LOW);
    lcd.print(F("WiFi ERR"));
    lcdClearLine(1);
    WiFi.begin(ssid, pass);
    Blynk.connect(10000);
    //server.begin();
  } else {
    digitalWrite(WIFILED, HIGH);
    lcd.print(F("WiFi OK"));
  }
}

//Check if log lines need to be synced from the SD card to google sheets
void recoveryManager() {
  if ((needRecovery == 1) && (WiFi.status() == WL_CONNECTED)) {
    recovery();
  }
}

//Delete all lines in Google Sheets Log
void resetSheets() {
  lcdClearLine(3);
  lcd.setCursor(4, 3);
  if (!client.connect(host, httpsPort)) {
    Serial.println(F("Connection failed"));
    lcd.print(F("CLOUD RESET FAIL"));
    return;
  }
  String url = "/macros/s/" + GAS_ID + "/exec?reset=1";

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println(F("Google Sheets Reset: SUCCESS"));
      lcd.print(F("CLOUD RESET SUCCESS"));
      break;
    }
  }
}

//Delete all lines in SD Log
void deleteSDLog() {
  lcd.setCursor(4, 3);
  bool fileRemoved = SD.remove("LOG.TXT");
  if (fileRemoved) {
    lcd.print(F("SD RESET OK"));
    Serial.println(F("File removed"));
  } else {
    lcd.print(F("SD RESET ERR"));
    Serial.println(F("Failed deleting file"));
    sdOK = false;
  }
}

//Clears line 'i' and moves the cursor back to the start of that line
void lcdClearLine(int i) {
  lcd.setCursor(0, i);
  lcd.print("                    ");
  lcd.setCursor(0, i);
}

/*
   Functions to manage stats and send reports have been split into separate functions to allow more flexibility:
   For example, you can send a mail recap if the date has changed (= on a new day), but you can also send a recap
   explicitly requesting it via control panel and so on
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
    humStat[0] = hum;
  }
  if (hum > humStat[1]) {
    humStat[1] = hum;
  }
}

//Resets all stats
void resetStats() {
  tempStat[0] = temp;
  tempStat[1] = temp;
  humStat[0] = hum;
  humStat[1] = hum;
  numMov = 0;
}


//Sends the report mail using Blynk. Body+Subject+emailaddress must be <140 Char
void sendReport() {
  String report = "Min-Max Temp: ";
  report += tempStat[0];
  report += "C - ";
  report += tempStat[1];
  report += "C | Min-Max Hum: ";
  report += humStat[0];
  report += " % - ";
  report += humStat[1];
  report += "% | #Movements: ";
  report += numMov;
  //After sending the email, stats get reset
  Blynk.email(F("Daily report"), report);
  resetStats();
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


/*checks if the date has changed and if it has, sends a report.
  It's handy having a separate function to do it because it can be called repeatedly in a timer
*/
void handleReports() {
  if (dateChanged())
    sendReport();
}
