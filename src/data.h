#ifndef DATA_H
#define DATA_H

#include "Arduino.h"

// Helper functions to convert floats to integers for easy transmission
#define FloatToInt16(f) (round((double)(f)*(double)100.0))

// Define constants used in the messages to help the receiving app
// Allow for future expansion of more than one message type
#define GATE_APP_ID 0x10A5 // Gate Sensor application
#define MSG_GATE 0x0000
#define MSG_GATE_VER 0x0000
#define MSG_HEARTBEAT 0x0001 // a Reading
#define MSG_HEARTBEAT_VER 0x0000 // a Reading

// Global variables to hold the sensor data
// These will be updated in the main sketch
unsigned long wdt_count = 0;
uint16_t battery = 0;
uint16_t solar = 0;
uint8_t charge = 0;

enum triggered_by {
    gate_closed,
    gate_opened,
    move_start,
    move_stop,
    heartbeat
} trigger;

// Enumerated type and variables for the state of the gate
enum gate_state {
    closed,
    open
} gate_state;
volatile enum gate_state gate;
volatile enum gate_state gate_previous;

// Enumerated type and variables for the state of the PIR
enum movement_state {
    quiet,
    detected
};
volatile enum movement_state movement;
volatile enum movement_state movement_previous;


typedef struct gateData {
    // Time since we started
    unsigned long millis;
    unsigned long watchdog_count;

    // Power
    uint16_t battery;
    uint16_t solar;
    uint8_t charge;

    // Gate Status
    uint8_t gate;
    uint8_t movement;

    // Trigger event
    uint8_t trigger;

} Gate_Data;

typedef struct tagXbeeCommBufferGate {

    // Header data
    uint16_t appID;
    uint16_t msgType;
    uint16_t version;

    // Data length
    uint16_t length; //could be just a byte, but keeping it on a 16 bit boundary helps decoding

    // The data itself
    Gate_Data data;
} Gate_Payload;

// Create a global instance of the payload which will be updated
// by the fillPayload() function below
Gate_Payload gate_payload;

void fill_gate_Payload (enum triggered_by trigger){
    gate_payload.appID = GATE_APP_ID;
    gate_payload.msgType = MSG_GATE;
    gate_payload.version = MSG_GATE_VER;
    gate_payload.length = sizeof(Gate_Data);
    gate_payload.data.millis = millis();
    gate_payload.data.watchdog_count = wdt_count;
    gate_payload.data.battery = battery;
    gate_payload.data.solar = solar;
    gate_payload.data.charge = charge;
    gate_payload.data.gate = gate;
    gate_payload.data.movement = movement;
    gate_payload.data.trigger = trigger;
}

#endif
