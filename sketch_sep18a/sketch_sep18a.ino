#define LED 13  // Normalmente è PIN 13 su arduino UNO PER IL LED INTERNO
#define LEDINTERNO 25  
void setup() {
  Serial.begin(9600);
  Serial.println(F("TEST"));  
  pinMode(LED, OUTPUT);      
}  
  
void loop() {  
  digitalWrite(LED, HIGH);    
  delay(1000);
  Serial.println(F("Dovrebbe Lampeggiare"));               
  digitalWrite(LED, LOW);    
  delay(1000);             
}  
