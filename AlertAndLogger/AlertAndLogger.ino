/*****************************************************************************************
    Lab of IoT - AA 2019/2020 - Emanuele Gargiulo
    Arduino Alert & Logger:
    DOCUMENTATION: https://drive.google.com/open?id=1mT9lr5-akYNQww7m9ZU5IvrvnQj9yaphkOifaIx3o74
    https://github.com/garjilius/ArduinoProjects/tree/master/AlertAndLogger
 *****************************************************************************************/
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

#define DHTPIN 2 //Temp/Hum sensor pin
#define IRPIN 9 //Motion sensor pin
#define chipSelect  8 //microSD card pin
#define WIFILED 3
#define SDLED 4

//#define DEBUG
//Allows toggling SERIAL PRINT on or off simply defining (or not defining) Debug (^ above).
//Disabling Debug saves about 2-3% of memory on Arduino
#ifdef DEBUG
#define DEBUG_PRINT(x)     Serial.print(x)
#define DEBUG_PRINTDEC(x)  Serial.print(x, DEC) //Print numbers to an arbitrary number of decimal places
#define DEBUG_PRINTLN(x)   Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTDEC(x)
#define DEBUG_PRINTLN(x)
#endif

//Used to identify the events (Humidity/Temperatyre/Movement) in the notification array
#define EVHUM 0
#define EVTEMP 1
#define EVMOV 2

//30000 byte = about 1000 lines. This defines the max size of log files
#define MAXLOGSIZE 30000

#define DHTTYPE DHT11     // DHT 11
//#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321 <--- Lab's Sensor

WiFiSSLClient client; //Used for communication with Google Sheets
WiFiServer server(80); //Used to Host the Control Panel

hd44780_I2Cexp lcd;
DHT dht(DHTPIN, DHTTYPE);
virtuabotixRTC myRTC(7, 6, 5); //Clock Pin Configuration

BlynkTimer timer; //Blynk's version of SimpleTimer. Used to setup repeated actions

String readString; //This strings will contain characters read from Clients that connect to Arduino's WebServer or from Google on upload to spreadsheet. Checking this response will allow us to understand if the request was successful
byte hum; //Last humidity value from DHT
float temp; //Last temperature value from DHT
int humLimit = 75; //Preset treshold for humidity. Can override via blynk app
int tempLimit = 25; //Preset treshold for temperature. Can override via blynk app
int timerGoogle; //a timer will be associated to this variable
int recoveredLines = 0; //Number of lines recovered while in recovery process (shown on LCD). 0 at all other times.
bool sdOK = false; //Will be set to true if SD is working.
long int logInterval = 900000L; //Preset log interval. Can override via arduino web server
int needRecovery = 0; //Number of log files that need to be recovered. Gets read from EEPROM to keep it safe when unplugged
bool notificationAllowed[3] = {true, true, true}; //Allows/Denies notifications for humidity, temperature and movement
bool systemDisabled = false; //Disables notifications for the whole system (via blynk app)

//Saving info used for recap email: Max&Min temp/hum + number of movements detected
//Position 0: Min - position 1: Max
byte humStat[2] = {100, 0};
float tempStat[2] = {100, -100};
int numMov = 0;
byte currentDay = 0;

//Google Sheets connection data
const char* host = "script.google.com";
const int httpsPort = 443;

//From arduino_secrets.h
char auth[] = BlynkToken; //Blynk Token: Pairs project with app
String GAS_ID = GoogleAppScript_ID; //Identifies Google App Script
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

void loop() {
  Blynk.run();  //Keeps blynk running
  timer.run(); //Blynk's "version" of SimpleTimer.h
  myRTC.updateTime();

  //:::::::::::::::::::::::::::::::::WEBSERVER:::::::::::::::::::::::::::::::
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) { //If a new client connects to the webservers
    //DEBUG_PRINTLN("new client");
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
          client.println();
          client.println(F("<HTML>"));
          client.println(F("<HEAD>"));
          client.println(F("<link rel='stylesheet' type='text/css' href='https://dl.dropbox.com/s/oe9jvh9pmyo8bek/styles.css?dl=0'/>")); //ATTACHED IN PROJECT FOLDER (STYLES.CSS)
          //Most of the page gets added via remote javascript to save space on arduino and speed things up
          client.println(F("<script src=\"https://dl.dropbox.com/s/cmtov3p8tj29wbs/jsextra.js?dl=0\"></script>")); //ATTACHED IN PROJECT FOLDER (JSEXTRA.JS)
          client.println(F("<script src=\"https://kit.fontawesome.com/a076d05399.js\"></script>"));
          client.println(F("<TITLE>Arduino Control Panel</TITLE>"));
          client.println(F("</HEAD>"));
          client.println(F("<BODY>"));
          client.println(F("<div id='mainBody'></div>"));
          client.println(F("<fieldset><legend><i class=\"far fa-clock\"></i> SET TIME <i class=\"far fa-clock\"></i></legend><form action="">"));
          client.println(F("<i class=\"fas fa-tachometer-alt\"></i> Logging Frequency (min)"));
          //:::::::::::Sets Text Area to LogInterval (in minutes):::::::::::::::
          int minInterval = logInterval / 60;
          minInterval = minInterval / 1000;
          String interval = "<input type=\"number\" name=\"logInterval\" min=\"1\" max=\"1440\" value=";
          interval += minInterval;
          interval += ">";
          //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
          client.println(interval);
          client.println(F("<input type=\"submit\" value=\"Set Log Interval\" class=\"button button1\"></form>"));
          client.println(F("<div id='time'></div></fieldset>"));
          client.println(F("<div id='reload'></div>"));
          client.println(F("<div id='leg'></div>"));
          client.println(F("</BODY>"));
          client.println(F("</HTML>"));
          // give the web browser time to receive the data
          delay(500);
          client.stop();
          //DEBUG_PRINTLN("client disconnected");

          //:::::Each string in the URL will be used to start a different function:::::::

          //Gets date from Client via Javascript (view JS source code) and sets RTC to it
          if (readString.indexOf("?date") > 0) {
            //day-month-year-hour-min-sec
            int date[7];
            //Splits the string at fixed places to get day, month etc
            date[6] = readString.substring(11, 13).toInt(); //dayofweek
            date[0] = readString.substring(14, 16).toInt(); //day
            date[1] = readString.substring(17, 19).toInt(); //month
            date[2] = readString.substring(20, 24).toInt(); //year
            date[3] = readString.substring(25, 27).toInt(); //hour
            date[4] = readString.substring(28, 30).toInt(); //min
            date[5] = readString.substring(31, 33).toInt(); //sec
            //Sets RTC: seconds, minutes, hours, day of the week, day of the month, month, year
            myRTC.setDS1302Time(date[5], date[4], date[3], date[6], date[0], date[1], date[2]);
            myRTC.updateTime(); //Need to update the RTC module before setting currentDay to its 'Day'
            currentDay = myRTC.dayofmonth;
            DEBUG_PRINTLN(F("Time Set"));
            lcdClearLine(3);
            lcd.print(F("Time Set"));
          }
          if (readString.indexOf("?recovery") > 0) {
            recoveryManager();
          }
          if (readString.indexOf("?logNow") > 0) {
            sendData();
          }
          if (readString.indexOf("?reset") > 0) {
            Blynk.email(F("Sheet Reset"), "Sheet Reset by Arduino");
            resetSheets();
          }
          if (readString.indexOf("?deleteSD") > 0) {
            deleteSDLog();
          }
          if (readString.indexOf("?sendReport") > 0) {
            DEBUG_PRINTLN("Send report...");
            sendReport();
          }
          if (readString.indexOf("?lcdoff") > 0) {
            lcd.off();
          }
          if (readString.indexOf("?lcdon") > 0) {
            lcd.on();
          }
          if (readString.indexOf("?lcdbacklightoff") > 0) {
            lcd.noBacklight();
          }
          if (readString.indexOf("?lcdbacklighton") > 0) {
            lcd.backlight();
          }
          if (readString.indexOf("?logInterval") > 0) {
            //Getting the useful data between startindex and endindex using indexOf
            int minInterval = readString.substring(readString.indexOf("=") + 1, readString.indexOf("HTTP") - 1).toInt();
            logInterval = 60L * 1000L * minInterval; //Force value to Long
            //To change the timer for logging, we have to delete it and set it up again
            timer.deleteTimer(timerGoogle);
            timerGoogle = timer.setInterval(logInterval, sendData);
            DEBUG_PRINT(F("Log Interval set to: "));
            DEBUG_PRINTLN(logInterval);
            lcdClearLine(3);
            lcd.print(F("Log Interval: "));
            lcd.print(minInterval);
          }
          readString = ""; //Reset readString
        }
      }
    }
  }
  //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
}

//Keep Blynk updated with lastest sensor data. Send notification if treshold passed or movement detected
void sendSensor() {
  readData();
  //If not connected to Blynk servers, avoid processing all the information. Still reads it to keep stats updated
  if (!Blynk.connected()) {
    return;
  }
  //Writing to Blynk's virtual pin sends data to the Blynk server (and app)
  Blynk.virtualWrite(V5, hum);
  Blynk.virtualWrite(V6, temp);

  //If system is disabled, return after updating hum and temp gauges in blynk app
  if (systemDisabled == 1) {
    return;
  }

  /*  Handle sensors' notifications.
      After Sending a notification for humidity or temperature,
      further notifications won't be sent if the temp/hum value hasn't gone at least once below the set treshold.
      For movement, an interval between two notification has been set to 1 minute.
  */
  if (hum > humLimit) {
    if (notificationAllowed[EVHUM] == true) {
      notificationAllowed[EVHUM] = false; //Disable notifications...
      String notifica = "Humidity is too high: ";
      notifica += hum;
      notifica += "% - ";
      notifica += printTime();
      Blynk.notify(notifica);
    }
  }
  else {
    notificationAllowed[EVHUM] = true; //Re-enable notifications if humidity went below threshold
  }

  if (temp > tempLimit) {
    if (notificationAllowed[EVTEMP] == true) {
      notificationAllowed[EVTEMP] = false; //Disable notifications...
      String notifica = "Temperature is too high: ";
      notifica += temp;
      notifica += " C - ";
      notifica += printTime();
      Blynk.notify(notifica);
    }
  }
  else {
    notificationAllowed[EVTEMP] = true; //Re-enable notifications if temperature went below threshold
  }

  if (digitalRead(IRPIN) == HIGH && notificationAllowed[EVMOV] == true) { //A movement was detected and notifications are allowed...
    notificationAllowed[EVMOV] = false;
    timer.setTimeout(60000L, enableMovementNotification); //Re-Enables movement notification after one minute
    numMov++;
    String notifica = "Movement Detected - ";
    notifica += printTime();
    Blynk.notify(notifica);
  }
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void setup() {
  Serial.begin(9600);
  pinMode(SDLED, OUTPUT);
  pinMode(WIFILED, OUTPUT);
  pinMode(DHTPIN, INPUT);
  pinMode(IRPIN, INPUT);
  WiFi.setTimeout(5000); //Wifi connection timeout, prevents attempt to connect to wifi being blocking
  dht.begin(); //DHT Initialization...
  server.begin();   // start the web server on port 80
  //Display Initialization
  lcd.begin(20, 4);
  //SD Initialization and handle sd status messages on display/serial
  checkSD();

  WiFi.begin(ssid, pass); //Connects to WiFi
  Blynk.config(auth); //Pair Blynk to the app

  /*Reads from eeprom if there's need for recovery or not.
    255 is EEPROM's default value.
    Checks if there's a file named 255.txt, given that recovery files get incremental names.
    If such file doesn't exist, we have confirmation that 255 is just the default value and set it back to 0.
    If the retrieved value is less than 0, it also means there's been a reading problem and the value is set to 0
  */
  EEPROM.get(0, needRecovery);
  if (needRecovery == 255 || needRecovery < 0) {
    if (!SD.exists("255.txt")) {
      needRecovery = 0;
      EEPROM.write(0, needRecovery);
    }
  }

  //Print number of files that need recovery
  DEBUG_PRINT(F("Need recovery: "));
  DEBUG_PRINTLN(needRecovery);

  //First logging happens 30s after boot, regardless of logging interval settings.
  timer.setTimeout(30000, sendData);
  myRTC.updateTime();
  handleDisplay(); //Display initialization
  currentDay = myRTC.dayofmonth; //Saving RTC current day on setup, so that I can compare realtimeclock day with the saved value and send a report on new day

  //Sets run frequency for used functions
  timer.setInterval(3000, sendSensor);
  timerGoogle = timer.setInterval(logInterval, sendData);
  timer.setInterval(10000, handleDisplay); //Display updated every 10 seconds
  timer.setInterval(35000, checkWifi); //WiFi status is checked every 35s
  timer.setInterval(1800000L, handleReports); //Need to send a report is checked every half an hour
  timer.setInterval(30000, checkSD); //Checks if SD is working every 30s
}
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

//Re Enables movement notifications.
//Need to have a specific function to do so as you need a void function to make "timer" work
void enableMovementNotification() {
  notificationAllowed[EVMOV] = true;
}

//Reads data from DHT sensor & Calls function to keep stats updated
void readData() {
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  if (isnan(hum) || isnan(temp)) { //Error reading from DHT Sensor
    DEBUG_PRINTLN(F("DHT Read Fail"));
    return;
  }
  manageStats(temp, hum); //Keep memory of highest/lowest temp/hum
}

//Logs data do Google Sheets
void sendData() {
  lcdClearLine(3);
  if ((WiFi.status() != WL_CONNECTED) || (!client.connect(host, httpsPort))) {
    DEBUG_PRINTLN(F("Connection failed"));
    lcd.print(F("CLOUD LOG ERR"));
    //Log data to SD IF AND ONLY IF logging to google has failed, to save space on microsd and computing power
    logData();
    return;
  }
  String url = "/macros/s/" + GAS_ID + "/exec?temp=" + temp + "&hum=" + hum;

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      DEBUG_PRINTLN(F("Logged to Google Sheets"));
      lcd.print(F("CLOUD LOG "));
      lcd.print(printTime());
      break;
    }
  }
}

//Delete all lines in Google Sheets Log
void resetSheets() {
  lcdClearLine(3);
  if (!client.connect(host, httpsPort)) {
    DEBUG_PRINTLN(F("Connection failed"));
    lcd.print(F("CLOUD RESET FAIL"));
    return;
  }
  String url = "/macros/s/" + GAS_ID + "/exec?reset";

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      DEBUG_PRINTLN(F("Sheets Reset: SUCCESS"));
      lcd.print(F("CLOUD RESET OK"));
      break;
    }
  }
}

//::::::::::::::::::::::::FOLLOWING FUNCTIONS HANDLE SD:::::::::::::::::::::::::::::::

//Checks if SD is working
//Retries to initialize SD if failed.
//If SD is not working, sd led comes up, then it is turned off again if SD starts working
void checkSD() {
  lcd.setCursor(8, 2);
  sdOK = SD.begin(chipSelect);
  if (!sdOK)  {
    lcd.print(F(" - SD ERR"));
    digitalWrite(SDLED, HIGH);  // indicate via LED
    sdOK = SD.begin(chipSelect); //retries initialization
  }
  else {
    digitalWrite(SDLED, LOW);  // indicate via LED
    lcd.print(F(" - SD OK "));
  }
}

//Logs data to SD
void logData() {
  if (!SD.begin(chipSelect)) {
    lcdClearLine(3);
    lcd.print("SD BACKUP FAIL");
    return; //If SD is not working, no point in trying to log to SD
  }
  //This string will contain all logging data for current sensors reading: Date/Time, Temp, Hum
  String dataString;
  dataString += printDate();
  dataString += " ";
  dataString += temp;
  dataString += " ";
  dataString += hum;

  File dataFile = SD.open(getLogFile(1), FILE_WRITE); //getLogFile gets the correct file name. Need it because log file have incremental names
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the Serial port too:
    DEBUG_PRINT(F("LoggedToSD"));
  }
  // if the file isn't open, pop up an error:
  else {
    DEBUG_PRINTLN(F("error opening log file"));
    sdOK = false;
  }
}

/*Gets the name of the right file to write:
  If the current file was getting too big, creates the next one
  If the file is just needed for reading, it returns the current file name
*/
String getLogFile(bool write) {
  if (write) { //if recovery file is needed in write mode, I might also need to increment needRecovery / create a new file, if no file existed or if current log file exceeds maximum size
    if (needRecovery == 0) {
      needRecovery++;
      EEPROM.write(0, needRecovery);
    }
    File checkFile = SD.open(String(needRecovery) += ".txt");
    if (checkFile.size() > MAXLOGSIZE) {
      needRecovery++;
      EEPROM.write(0, needRecovery);
    }
    checkFile.close();
  }
  DEBUG_PRINTLN(String(needRecovery) += ".txt");
  return String(needRecovery) += ".txt";
}

//Delete all SD Log files
void deleteSDLog() {
 if(!SD.begin(chipSelect)) {
  return; //No point in trying to delete files if the SD module is not working
 }
  lcdClearLine(3);
  File root = SD.open("/");
  //Delete every file. It obviously assumes all files on the SD card are recovery logs.
  //No check is made on files to save memory on arduino
  while (true) {
    File entry =  root.openNextFile();
    if (!entry) {
      // no more files, SD Empty
      lcd.print("All Files del.");
      needRecovery = 0; //It should already be 0 as it is decreased with each file removed/recovered, but initializing ensures that if needRecovery had a wrong value, it will be back to 0 after deleteSDlog
      EEPROM.write(0, needRecovery);
      break;
    }
    entry.close();
    DEBUG_PRINT(entry.name());
    if (SD.remove(entry.name()) && needRecovery > 0) {
      DEBUG_PRINTLN(": removed");
      needRecovery--;
    }
    else {
      DEBUG_PRINTLN(": not removed");
    }
  }
}


/*
   This functions allows uploading to Google Sheets data that had only been logged to the SD card due to a lack of connection
*/
void recovery() {
  File myFile = SD.open(getLogFile(0));
  if (myFile) {
    while (myFile.available()) {
      //Every line contains date, temperature and humidity. I get the line and then split it in the values I need
      String dateS = myFile.readStringUntil(' ');
      String tempS = myFile.readStringUntil(' ');
      String humS = myFile.readStringUntil('\n');

      if (!client.connect(host, httpsPort)) {
        DEBUG_PRINTLN(F("Recovery failed"));
        lcdClearLine(3);
        lcd.print(F("Recovery FAILED"));
        return;
      }
      String url = "/macros/s/" + GAS_ID + "/exec?temp=" + tempS + "&hum=" + humS + "&date=" + dateS;
      url.replace("\n", ""); //Removes newlines
      url.replace("\r", "");
      DEBUG_PRINTLN(F(url));

      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Connection: close\r\n\r\n");

      while (client.connected()) {
        String line = client.readStringUntil('\n');
        //DEBUG_PRINTLN(line);
        if (line == "\r") {
          recoveredLines++;
          DEBUG_PRINTLN(F("Line recovered"));
          lcdClearLine(3);
          lcd.print(recoveredLines);
          lcd.print(F(" log recovered"));
          break;
        }
      }
    }
    myFile.close();
    if (SD.remove(myFile.name()))
      DEBUG_PRINTLN("REMOVED OK");
    //If going backwards I haven't reached file n 0, I keep going backwards
    needRecovery--;
    if (needRecovery > 0) {
      recovery();
    } else { //Recovery Done
      DEBUG_PRINTLN(F("Successful Recovery"));
      lcdClearLine(3);
      recoveredLines = 0;
      lcd.print(F("Recovery OK"));
    }
  } else {
    lcdClearLine(3);
    lcd.print(F("Recovery FAILED"));
  }
  //Saving the updated number of files that need recovery to EEPROM
  EEPROM.write(0, needRecovery);
}


//:::::::::Following Functions read from Blynk's server values to be re-synced to arduino when rebooted::::::::::::
//On connection to Blynk Server
BLYNK_CONNECTED() {
  // Request Blynk server to re-send latest values for all pins
  Blynk.syncAll();
}

//Import variables from virtual pin status
BLYNK_WRITE(V0)  {
  systemDisabled = param.asInt();
}

BLYNK_WRITE(V3)  {
  tempLimit = param.asFloat();
}

BLYNK_WRITE(V2)  {
  humLimit = param.asFloat();
}

//:::::::::::::::FOLLOWING FUNCTIONS HANDLE TIME&DATE STRINGS:::::::::::::
//Turns a single digit number in a two characters string, or number in string if >=2 digits
String twoDigits(uint8_t input) {
  if (input < 10) {
    String inputString = String(input);
    String zero = "0";
    return zero += inputString;
  }
  return String(input);
}

//Returns time string in HH:MM format
String printTime() {
  String orario = "";
  orario += twoDigits(myRTC.hours);
  orario += ":";
  orario += twoDigits(myRTC.minutes);
  return orario;
}

//Returns date string in DD/MM/YYYY-HH.MM.SS format.
//ANY change will require handling the new format in Google App Script when logging to Google Sheets
String printDate() {
  String orario = "";
  orario += twoDigits(myRTC.dayofmonth);
  orario += "/";
  orario += twoDigits(myRTC.month);
  orario += "/";
  orario += myRTC.year;
  orario += "-";
  orario += twoDigits(myRTC.hours);
  orario += ".";
  orario += twoDigits(myRTC.minutes);
  orario += ".";
  orario += twoDigits(myRTC.seconds);
  return orario;
}
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

//Handles info on the LCD i2c display
//This function assumes a 20x4 display
void handleDisplay() {
  lcd.setCursor(0, 0);
  lcd.print(printTime());
  lcd.print(" H:");
  lcd.print(hum);
  lcd.print("% T:");
  lcd.print(temp);
  lcd.print("C");
  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 1);
    lcd.print(F("IP: "));
    lcd.print(WiFi.localIP());
    lcd.setCursor(0, 2);
    lcd.print(F("WiFi OK "));
  } else {
    lcd.setCursor(0, 2);
    lcd.print(F("WiFi ERR"));
    lcdClearLine(1); //If WiFi is not working, no point in printing an ip address (it would be 0.0.0.0)
  }
  lcd.setCursor((20-String(needRecovery).length()-2), 3); //Dynamic position to account for the fact that needrecovery might have a different number of digits.
  lcd.print(F("("));
  lcd.print(needRecovery); //Number of files that need recovery
  lcd.print(F(")"));
}

//If WIFI is not working, we attempt to reconnect to wifi and to blynk servers. This function is called periodically
void checkWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WIFILED, LOW);
    WiFi.begin(ssid, pass);
  } else {
    digitalWrite(WIFILED, HIGH);
  }
  //If WIFI is connected but blynk isn't, I can try to reconnect to blynk servers
  if ((WiFi.status() == WL_CONNECTED) && (!Blynk.connected())) {
    Blynk.connect(10000); //10K = Connection Timeout
  }
}

//Clears line 'i' and moves the cursor back to the start of that line
void lcdClearLine(int i) {
  lcd.setCursor(0, i);
  lcd.print("                    "); //20 Whitespaces to clean the whole line on a 20x4 display
  lcd.setCursor(0, i);
}

//Check if log lines need to be synced from the SD card to google sheets
void recoveryManager() {
  if (needRecovery >= 1 && sdOK) {
    //Start recovery process if files need it
    recovery();
  }
  else {
    lcdClearLine(3);
    lcd.print("No Recovery!");
  }
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

//Resets all stats. Temp and humidity max and minimum all initializated to current value
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
  report += "% - ";
  report += humStat[1];
  report += "% | #Movements: ";
  report += numMov;
  //After sending the email, stats get reset
  Blynk.email(F("Daily report"), report);
  lcdClearLine(3);
  lcd.print("Report Sent");
  resetStats();
}

//If the date has changed, returns true and updates currentDay
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
//:::::::::::::::::::::::::::::::::EOF:::::::::::::::::::::::::::::::::::::::::::
