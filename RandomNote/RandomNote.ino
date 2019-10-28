#define pinBuzzer 13
#define pResistor A0
#define NUMLED 4
int led[NUMLED] = {8,9,10,11};
float pResistorValue;

void setup() {
  // put your setup code here, to run once:
  pinMode(pinBuzzer,OUTPUT);
  pinMode(pResistor,INPUT);
  
  for(int i = 0; i <=NUMLED; i++) {
    pinMode(led[i],OUTPUT);
  }

  Serial.begin(9600);
}

void loop() {
    pResistorValue = analogRead(pResistor);
    //Serial.println(pResistorValue);

    /*
  if(pResistorValue < 500) {
    play();
  } else {
    ledsOff();
  }
  */
  play();
}

void ledOn(int lednum) {
    ledsOff();
    Serial.println(lednum);
    digitalWrite(led[lednum],HIGH);
}

void play() {
  int nota = random(200,1000);
  //int nota = random((1000 - pResistorValue) / 3,1000);
  tone(pinBuzzer,nota,100);
  delay(random(pResistorValue/3,pResistorValue/1.5));
  ledOn(random(0,NUMLED));
}

void ledsOff() {
   for(int i = 0; i <=NUMLED; i++) {
    digitalWrite(led[i],LOW);
  }
}
