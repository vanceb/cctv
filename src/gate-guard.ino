#include <avr/sleep.h>
#include <avr/wdt.h>

#include "data.h"
#include "comms.h"

// Pins
#define PIR_PIN 2
#define HALL_PIN 3
#define BATTERY_PIN A7
#define SOLAR_PIN A2

// Timing and Debounce intervals
int waitTime = 2000;
int sleep_delay = 10000;
unsigned long now = 0;
volatile unsigned long last_activity = 0;

// Watchdog Timer Flag
volatile bool wdt_flag = false;
int heartbeat_count = 8;

// Global variables for state
volatile int hall;
volatile int hall_previous;
volatile int pir;

// Variable to debounce the PIR
unsigned long pir_last_triggered = millis();

void enable_pir_interrupt() {
    attachInterrupt(digitalPinToInterrupt(PIR_PIN), pir_triggered, FALLING);
}


void disable_pir_interrupt() {
    detachInterrupt(digitalPinToInterrupt(PIR_PIN));
}


// watchdog interrupt
ISR (WDT_vect)
{
   wdt_disable();  // disable watchdog
   wdt_flag = true;
}  // end of WDT_vect


// Interrupt handler for the PIR
void pir_triggered() {
    // cancel sleep as a precaution
    sleep_disable();
    pir_last_triggered = millis();
    last_activity = millis();
}


// Interrupt handler for the Hall Effect sensor
void gate_triggered() {
    // cancel sleep as a precaution
    sleep_disable();
    hall = digitalRead(HALL_PIN);
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
    last_activity = millis();
}

// Notication functions
void notify_gate(enum gate_state gate) {
    if (gate == open) {
        fill_gate_Payload(gate_opened);
    } else {
        fill_gate_Payload(gate_closed);
    }
    xbeeSend(zbTxGt);
}


void notify_movement (enum movement_state movement) {
    if (movement == detected) {
        fill_gate_Payload(move_start);
    } else {
        fill_gate_Payload(move_stop);
    }
    xbeeSend(zbTxGt);
}


// Sleep routines
void go_to_sleep() {
    // Give a chance for any remaining data to be sent
    delay(1000);

    // Send the radio to sleep
    sleepXBee();

    // disable ADC
     ADCSRA = 0;

     // clear various "reset" flags
    MCUSR = 0;  // allow changes, disable reset
     WDTCSR = bit (WDCE) | bit (WDE);
     // set interrupt mode and an interval
     WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0);    // set WDIE, and 1 second delay
     wdt_reset();  // pat the dog


     set_sleep_mode (SLEEP_MODE_PWR_DOWN);
     sleep_enable();

     // Do not interrupt before we go to sleep, or the
     // ISR will detach interrupts and we won't wake.
     noInterrupts ();

     // will be called when pin D2 goes low
     //attachInterrupt (0, wake, FALLING);
     //EIFR = bit (INTF0);  // clear flag for interrupt 0

     // turn off brown-out enable in software
     // BODS must be set to one and BODSE must be set to zero within four clock cycles
     MCUCR = bit (BODS) | bit (BODSE);
     // The BODS bit is automatically cleared after three clock cycles
     MCUCR = bit (BODS);

     // We are guaranteed that the sleep_cpu call will be done
     // as the processor executes the next instruction after
     // interrupts are turned on.
     interrupts ();  // one cycle
     sleep_cpu ();   // one cycle
    // cancel sleep as a precaution
    sleep_disable();
    wdt_disable();
    wakeXBee();
    // Give a chance for everything to settle
    delay(50);
    //Serial.println("Awake");
}


// the setup routine runs once when you press reset:
void setup() {
    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);
    pinMode(HALL_PIN, INPUT);
    pinMode(PIR_PIN, INPUT);

    // Get initial values for the power readings
    battery = analogRead(BATTERY_PIN);
    solar = analogRead(SOLAR_PIN);

    // Force the correct setting of the PIR interrupt by faking gate state change
    hall = digitalRead(HALL_PIN);
    if (hall == 1) {
        hall_previous = 0;
    } else {
        hall_previous = 1;
    }
    gate_triggered();
    movement = quiet;
    movement_previous = quiet;

    // Setup interrupt for the hall switch
    attachInterrupt(digitalPinToInterrupt(HALL_PIN), gate_triggered, CHANGE);
}


void loop() {
    // What time is it for this loop?
    now = millis();

    // If the gate is open then use the PIR sensor to detect movement
    if ( gate == open ) {
        // Debounce PIR input
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

    // Do timed routines based on number of watchdog timer interrupts
    if (wdt_flag) {
        wdt_flag = false;
        wdt_count++;
        if (wdt_count % heartbeat_count == 0) {
            battery = analogRead(BATTERY_PIN);
            solar = analogRead(SOLAR_PIN);
            fill_gate_Payload(heartbeat);
            xbeeSend(zbTxGt);
        }
    }

    if (now > last_activity + sleep_delay) {
        go_to_sleep();        // delay in between reads for stability
    }
    delay(1);
}
