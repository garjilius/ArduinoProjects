/*
Due led che lampeggiano alternativamente rallentando col passare del tempo.
Quando l'attesa arriva a 5s, si resetta
*/

#define LED_BUILTIN 13
#define RED_LED 3
#define YELLOW_LED 9
#define restartdelay 5000

int wait;
// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  wait = 0;
}

// the loop function runs over and over again forever
void loop() {
  wait +=1000;

    if(wait > 5000) {
        wait = 0;
        Serial.println("Ho Resettato il contatore. Tra 5s si riparte!");
        //digitalWrite(LED_BUILTIN,HIGH);
        blink(LED_BUILTIN, 250,restartdelay);
        digitalWrite(RED_LED, LOW);
        digitalWrite(YELLOW_LED, LOW); //SPENGO ENTRAMBI I LED PRINCIPALI 
        digitalWrite(LED_BUILTIN,LOW);
        return;
  }
  
  Serial.println(wait);
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
