/*
  WiFi Web Server LED Blink

 A simple web server that lets you blink an LED via the web.
 This sketch will print the IP address of your WiFi module (once connected)
 to the Serial monitor. From there, you can open that address in a web browser
 to turn on and off the LED on pin 11.

 If the IP address of your board is yourAddress:
 http://yourAddress/H turns the LED on
 http://yourAddress/L turns it off

 This example is written for a network using WPA encryption. For
 WEP or WPA, change the Wifi.begin() call accordingly.

 Circuit:
 * Board with NINA module (Arduino MKR WiFi 1010, MKR VIDOR 4000 and UNO WiFi Rev.2)
 * LED attached to pin 11

 created 25 Nov 2012
 by Tom Igoe
 */
#include <SPI.h>
#include <WiFiNINA.h>
#define led1 11

#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)
String readString;


int status = WL_IDLE_STATUS;
WiFiServer server(80);

void setup() {
  Serial.begin(9600);      // initialize serial communication
  pinMode(11, OUTPUT);      // set the LED pin mode

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.print("Please upgrade the firmware. Installed: ");
    Serial.println(fv);
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();                           // start the web server on port 80
  printWifiStatus();                        // you're connected now, so print out the status
}


void loop() {
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
           Serial.println(readString); //scrivi sul monitor seriale per debugging

           //QUESTO LO SOSTITUISCO CON LA PAGINA WEB DELLA SD
           client.println("HTTP/1.1 200 OK"); //Invio nuova pagina
           client.println("Content-Type: text/html");
           client.println();     
           client.println("<HTML>");
           client.println("<HEAD>");
           client.println("<meta name='apple-mobile-web-app-capable' content='yes' />");
           client.println("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");
           client.println("<link rel='stylesheet' type='text/css' href='http://www.progettiarduino.com/uploads/8/1/0/8/81088074/style3.css' />");
           client.println("<TITLE>Control Panel</TITLE>");
           client.println("</HEAD>");
           client.println("<BODY>");
           client.println("<H1>Control Panel</H1>");
           client.println("<hr />");
           client.println("<br />");  
           client.println("<H2>Welcome, Emanuele</H2>");
           client.println("<br />");  
           client.println("<a href=\"/?button1on\"\">ON - LED 1</a>");          //Modifica a tuo piacimento:"Accendi LED 1"
           client.println("<a href=\"/?button1off\"\">OFF - LED 1</a><br />");    //Modifica a tuo piacimento:"Spegni LED 1" 
           client.println("<br />");  
           client.println("<br />");

           client.println("<br />"); 
           client.println("</BODY>");
           client.println("</HTML>");
     
           delay(1);
           client.stop();
           //Controlli su Arduino: Se è stato premuto il pulsante sul webserver
           //QUI CI VA TUTTA LA LOGICA CHE GESTISCE GLI INPUT
           if (readString.indexOf("?button1on") >0){
               digitalWrite(led1, HIGH);
           }
           if (readString.indexOf("?button1off") >0){
               digitalWrite(led1, LOW);
           }
           if (readString.indexOf("?button2on") >0){
               digitalWrite(led1, HIGH);  
           }
           if (readString.indexOf("?button2off") >0){
               digitalWrite(led1, LOW);
           }
           if (readString.indexOf("?button3on") >0){
               digitalWrite(led1, HIGH);  
           }
           if (readString.indexOf("?button3off") >0){
               digitalWrite(led1, LOW);
           }
           if (readString.indexOf("?button4on") >0){
               digitalWrite(led1, HIGH);  
           }
           if (readString.indexOf("?button4off") >0){
               digitalWrite(led1, LOW);
           }

            //Cancella la stringa una volta letta
            readString="";  
           
         }
       }
    }
}
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}
