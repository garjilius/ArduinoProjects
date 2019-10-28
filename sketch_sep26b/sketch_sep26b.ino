#include "DHT.h"

#define pResistor A0
#define DHTPIN 2     // Il pin a cui è collegato il sensore
#define LIGHTLED 10
#define HUMIDITYLED 9
#define TEMPLED 8
#define BUZZER 11
#define OKLED 12

#define TEMPLIMIT 25.80
#define HUMLIMIT 70
#define LIGHTLIMIT 600

#define DHTTYPE DHT11   // DHT 11 
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);
int i;
char instruction;

void setup() {
  Serial.begin(9600); 
  pinMode(pResistor,INPUT);
  pinMode(LIGHTLED,OUTPUT);
  pinMode(HUMIDITYLED,OUTPUT);
  pinMode(TEMPLED,OUTPUT);
  pinMode(BUZZER,OUTPUT);
  pinMode(OKLED,OUTPUT);
  dht.begin();
}

void loop() {
  instruction = Serial.read();
  if (instruction == 't') {
      digitalWrite(TEMPLED,HIGH);
      Serial.println("Acceso led temperatura");
      delay(2000);
  } else if (instruction == 'h') {
      digitalWrite(HUMIDITYLED,HIGH);
      Serial.println("Acceso led umidità");
      delay(2000);
  } else if (instruction == 'l') {
      digitalWrite(LIGHTLED,HIGH);
      Serial.println("Acceso led luce");
      delay(2000);
  } else if (instruction == 's') {
      digitalWrite(LIGHTLED,LOW);
      digitalWrite(HUMIDITYLED,LOW);
      digitalWrite(TEMPLED,LOW);
      digitalWrite(OKLED,LOW);
      delay(2000);
  } else if (instruction == 'a') {
      digitalWrite(TEMPLED,HIGH);
      digitalWrite(HUMIDITYLED,HIGH);
      digitalWrite(LIGHTLED,HIGH);
      digitalWrite(OKLED,HIGH);
      Serial.println("Accesi tutti i led");
      delay(2000);
  }
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float pResistorValue = analogRead(pResistor);

/*
if(t > (TEMPLIMIT + 0.10)) {
for(i=0;i<10;i++)
  {
    digitalWrite(BUZZER,HIGH);
    delay(1);//wait for 1ms
    digitalWrite(BUZZER,LOW);
    delay(1);//wait for 1ms
  }
}
*/

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

        if(pResistorValue < LIGHTLIMIT) { //Se c'è meno luce di una certa soglia, accende led
          digitalWrite(LIGHTLED, HIGH);
    }
    else {
          digitalWrite(LIGHTLED, LOW);
    } 
        if(h > HUMLIMIT) { //Se umidità più alta di una soglia, accende led
          digitalWrite(HUMIDITYLED, HIGH); 
    }
    else { 
          digitalWrite(HUMIDITYLED, LOW);
    }
            if(t >= TEMPLIMIT) { //Se temperatura più alta di una certa soglia, accende led
          digitalWrite(TEMPLED, HIGH);
    }
    else {
          digitalWrite(TEMPLED, LOW);
    }
  }
  if(tempOk(t,24,27) && humOk(h,50,70) && lightOk(pResistorValue,500,900)) {
    //Serial.println("TUTTO OK");
    digitalWrite(OKLED,HIGH);
  } else {
    digitalWrite(OKLED,LOW);
  }
    delay(1000); //ALLA FINE DEL LOOP; DELAY PRIMA DEL PROSSIMO LOOP
}

bool tempOk(float TEMP,float MINTEMP,float MAXTEMP) {
  if(TEMP >= MINTEMP && TEMP <= MAXTEMP)
    return true;
  else
    return false;
}

bool humOk(float HUM, float MINHUM, float MAXHUM) {
 if(HUM >= MINHUM && HUM <= MAXHUM)
    return true;
  else
    return false;
}

bool lightOk(int LIGHT,int MINLIGHT,int MAXLIGHT) {
 if(LIGHT >= MINLIGHT && LIGHT <= MAXLIGHT)
    return true;
  else
    return false;
}
