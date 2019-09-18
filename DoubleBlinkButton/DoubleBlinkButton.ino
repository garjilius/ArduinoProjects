/*
Due led che lampeggiano alternativamente rallentando col passare del tempo.
Quando l'attesa arriva a 5s, si resetta
*/

#define LED_BUILTIN 13
#define RED_LED 3
#define YELLOW_LED 9
#define restartdelay 5000
#define button 5

int wait;
bool spento;
// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(button, INPUT);  //Il pin del pulsante è un'entrata 


  wait = 0;
}

// the loop function runs over and over again forever
void loop() {
spento = digitalRead(button);  //Lettura del pulsante  
if(spento) {
  reset();
  return;
}
  wait +=1000;

    if(wait > 5000) {
      reset();
  }
  
  Serial.println(wait);
  digitalWrite(LED_BUILTIN,LOW);
  digitalWrite(RED_LED, HIGH);
  digitalWrite(YELLOW_LED, LOW);
  delay(wait);                       
  digitalWrite(RED_LED, LOW);   
  digitalWrite(YELLOW_LED, HIGH);    
  delay(wait);                       

}

void blink(int led, int ritardo, int durata) {
  int timeleft = durata;
  while(timeleft>0) {
    digitalWrite(led,HIGH);
    delay(ritardo);
    digitalWrite(led,LOW);
    delay(ritardo);
    timeleft -= ritardo*2;
    }
}

void reset() {
        wait = 0;
        Serial.println("Ho Resettato il contatore. Tra 5s si riparte!");
        //digitalWrite(LED_BUILTIN,HIGH);
        digitalWrite(RED_LED, LOW);
        digitalWrite(YELLOW_LED, LOW); //SPENGO ENTRAMBI I LED PRINCIPALI 
        digitalWrite(LED_BUILTIN,LOW);
        blink(LED_BUILTIN, 250,restartdelay);
        return;
}
