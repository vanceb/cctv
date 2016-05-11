int pirPin = 2;
int hallPin = 3;

int waitTime = 1000;
unsigned long now = 0;

int hall;
int pir;
int hall_prev;
bool move_state = false;
bool movement = false;

unsigned long millis_last_triggered = millis();

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(pirPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(pirPin), moved, FALLING);
  pinMode(hallPin, INPUT);
}

void moved() {
    millis_last_triggered = millis();
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);
  int hall = digitalRead(hallPin);
  int pir = digitalRead(pirPin);
  if (hall != hall_prev){
      if (hall == 0){
          Serial.println("Gate closed");
      } else {
          Serial.println("Gate open");
      }
      hall_prev = hall;
  }

  now = millis();
  if (now > (millis_last_triggered + waitTime)){
      movement = false;
  } else {
      movement = true;
  }

  if (move_state != movement){
      Serial.print(millis());
      move_state = movement;
      if (movement){
          Serial.println(": Movement detected");
          Serial.println(sensorValue);
      } else {
          Serial.println(": All Quiet");
      }
  }

  delay(1);        // delay in between reads for stability
}
