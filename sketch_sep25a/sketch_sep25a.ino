#include "DHT.h"

#define pResistor A0
#define DHTPIN 2     // Il pin a cui è collegato il sensore
#define CONTROLED 11
#define HUMIDITYLED 4
#define TEMPLED 8

//#define DHTTYPE DHT11   // DHT 11 
#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600); 
  pinMode(pResistor,INPUT);
  pinMode(CONTROLED,OUTPUT);
  pinMode(HUMIDITYLED,OUTPUT);
  pinMode(TEMPLED,OUTPUT);
  dht.begin();
}

void loop() {

  delay(1000);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float pResistorValue = analogRead(pResistor);


  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT");
  } else {
    Serial.print("Humidity: "); 
    Serial.print(h); // Stampa nel seriale la percentuale dell'umidità
    //Serial.print(" %t");
    Serial.print(" Temperature: "); 
    Serial.print(t); // Stampa nel seriale il valore della temperatura
    Serial.print(" *C");
    Serial.print(" Photoresistence: ");
    Serial.println(pResistorValue);

        if(pResistorValue <300) { //Se c'è meno luce di una certa soglia, accende led
          digitalWrite(CONTROLED, HIGH);
    }
    else {
          digitalWrite(CONTROLED, LOW);
    } 
        if(h > 80) { //Se umidità più alta di una soglia, accende led
          digitalWrite(HUMIDITYLED, HIGH); 
    }
    else { 
          digitalWrite(HUMIDITYLED, LOW);
    }
            if(t >= 27.40) { //Se temperatura più alta di una certa soglia, accende led
          digitalWrite(TEMPLED, HIGH);
    }
    else {
          digitalWrite(TEMPLED, LOW);
    }
  }
}
