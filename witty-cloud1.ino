
int fotoZellePin = A0; //Bestimmt das der Fotowiderstand an den analogen PIN A0 angeschlossen ist
#define RED_LED D8
#define GREEN_LED D6
#define BLUE_LED D7
#define CONTROL_LED D4 //Blaue SMD LED am ESP Modul
const int BUTTON = 4;

void setup() {
  Serial.begin(9600);
  pinMode(BLUE_LED, OUTPUT);   /* Setzt den PIN für die blaue LED als Ausgang. */
  pinMode(RED_LED, OUTPUT);    /* Setzt den PIN für die rote LED als Ausgang. */
  pinMode(GREEN_LED, OUTPUT);  /* Setzt den PIN für die grüne LED als Ausgang. */
  pinMode(CONTROL_LED, OUTPUT);  /* Setzt den PIN für die grüne LED als Ausgang. */
}

void loop() {
  time_t now = time(nullptr);
  Serial.println(ctime(&now));

  int value = analogRead(fotoZellePin); // 1024->sehr hell, 166->Zimmerhelligkeit
  int buttonValue = digitalRead(BUTTON);
  
  if (buttonValue==LOW) {
    analogWrite(BLUE_LED,255);
    digitalWrite(CONTROL_LED, LOW);
    analogWrite(RED_LED,0);
    analogWrite(GREEN_LED,0);
  } else if (value>100) {
    analogWrite(BLUE_LED,0);
    analogWrite(RED_LED,255);
    analogWrite(GREEN_LED, 0);
    digitalWrite(CONTROL_LED, HIGH);
  } else {
    analogWrite(BLUE_LED,0);
    analogWrite(RED_LED,0);
    analogWrite(GREEN_LED, 255);
    digitalWrite(CONTROL_LED, HIGH);
  }
  
  delay(550);
}
