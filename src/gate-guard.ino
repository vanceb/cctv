// Pins
int pirPin = 2;
int hallPin = 3;

// Timing and Debounce intervals
int waitTime = 2000;
unsigned long now = 0;

// Global variables for state
volatile int hall;
volatile int hall_previous;
volatile int pir;

// Enumerated type and variables for the state of the gate
enum gate_state {
    open,
    closed
} gate_state;
volatile enum gate_state gate;
volatile enum gate_state gate_previous;

// Enumerated type and variables for the state of the PIR
enum movement_state {
    detected,
    quiet
};
volatile enum movement_state movement;
volatile enum movement_state movement_previous;

// Variable to debounce the PIR
unsigned long pir_last_triggered = millis();


void enable_pir_interrupt() {
    attachInterrupt(digitalPinToInterrupt(pirPin), pir_triggered, FALLING);
}


void disable_pir_interrupt() {
    detachInterrupt(digitalPinToInterrupt(pirPin));
}


// Interrupt handler for the PIR
void pir_triggered() {
    pir_last_triggered = millis();
}


// Interrupt handler for the Hall Effect sensor
void gate_triggered() {
    hall = digitalRead(hallPin);
    if (hall != hall_previous) {
        // The state has changed
        if (hall == 1) {
            gate = open;
            enable_pir_interrupt();
        } else {
            gate = closed;
            disable_pir_interrupt();
            movement = quiet;
        }
        hall_previous = hall;
    }
}

// Notication functions
void notify_gate(enum gate_state gate) {
    if (gate == open) {
        Serial.println("Gate Open");
    } else {
        Serial.println("Gate Closed");
    }
}


void notify_movement (enum movement_state movement) {
    if (movement == detected) {
        Serial.println("Movement Detected");
    } else {
        Serial.println("All Quiet");
    }
}


// the setup routine runs once when you press reset:
void setup() {
    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);
    pinMode(hallPin, INPUT);
    pinMode(pirPin, INPUT);

    // Force the correct setting of the PIR interrupt by faking gate state change
    hall = digitalRead(hallPin);
    if (hall == 1) {
        hall_previous = 0;
    } else {
        hall_previous = 1;
    }
    gate_triggered();
    movement = quiet;
    movement_previous = quiet;

    // Setup interrupt for the hall switch
    attachInterrupt(digitalPinToInterrupt(hallPin), gate_triggered, CHANGE);
}


void loop() {
    // read the input on analog pin 0:
    //int sensorValue = analogRead(A0);

    // If the gate is open then use the PIR sensor to detect movement
    if ( gate == open ) {
        // Debounce PIR input
        now = millis();
        if (now > (pir_last_triggered + waitTime)){
            movement = quiet;
        } else {
            movement = detected;
        }
        if (movement != movement_previous) {
            notify_movement(movement);
            movement_previous = movement;
        }
    }

    // Check for gate state change and notify if necessary
    if (gate != gate_previous) {
        notify_gate(gate);
        gate_previous = gate;
    }

    delay(1);        // delay in between reads for stability
}
