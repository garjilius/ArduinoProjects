#define pinBuzzer 13
#define pResistor A0
#define NUMLED 4
int led[NUMLED] = {8,9,10,11};

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
    float pResistorValue = analogRead(pResistor);
    //Serial.println(pResistorValue);
  if(pResistorValue < 500) {
    play();
  } else {
    ledsOff();
  }
}

void ledOn(int lednum) {
    ledsOff();
    Serial.println(lednum);
    digitalWrite(led[lednum],HIGH);
}

void play() {
  int nota = random(200,1000);
  tone(pinBuzzer,nota,100);
  delay(random(100,250));
  ledOn(random(0,NUMLED));
}

void ledsOff() {
   for(int i = 0; i <=NUMLED; i++) {
    digitalWrite(led[i],LOW);
  }
}
