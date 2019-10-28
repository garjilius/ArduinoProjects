#define ledYellow 7
#define ledRed 5
#define ledGreen 6
#define buttonState 4
#define internalLed 25

int statoBottone;
void setup() {
  Serial.begin(9600);
  Serial.println("TEST");  
  pinMode(ledYellow, OUTPUT);   
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(buttonState, INPUT);
  
}  
  
void loop() {  

  /*
   * Leggo il bottone, se è premuto faccio accendere i led in sequenza
   */
   statoBottone = digitalRead(buttonState);
 
   digitalWrite(ledYellow,LOW);
   digitalWrite(ledGreen,LOW);
   digitalWrite(ledRed,LOW);

  Serial.println(statoBottone);
  //se il bottone è premuto
  if(statoBottone == HIGH) {
    blink(ledYellow);
    blink(ledGreen);
    blink(ledRed);
  }
   
           
}  

void blink(int pin) {
  digitalWrite(pin,HIGH);
  delay(200);
  digitalWrite(pin,LOW);
  delay(200);
}
