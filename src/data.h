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

enum triggered_by {
    trigger_gate,
    trigger_movement
} trigger;

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

// Type to be used for transmission over xbee
typedef struct heartbeatData {
    // Time since we started
    unsigned long millis;
    unsigned long watchdog_count;

    // Power
    uint16_t battery;
    uint16_t solar;

} Heartbeat;

typedef struct tagXbeeCommBuffer {

    // Header data
    uint16_t appID;
    uint16_t msgType;
    uint16_t reserved; // included as it maked decoding easier by placing the header on a 32 bit boundary

    // Data length
    uint16_t length; //could be just a byte, but keeping it on a 16 bit boundary helps decoding

    // The data itself
    Heartbeat data;

} HB_Payload;

// Create a global instance of the payload which will be updated
// by the fillPayload() function below
HB_Payload hb_payload;

void fill_HB_Payload (){
    hb_payload.appID = GATE_APP_ID;
    hb_payload.msgType = MSG_HEARTBEAT;
    hb_payload.reserved = MSG_HEARTBEAT_VER;
    hb_payload.length = sizeof(Heartbeat);
    hb_payload.data.millis = millis();
    hb_payload.data.watchdog_count = wdt_count;
    hb_payload.data.battery = battery;
    hb_payload.data.solar = solar;
}

typedef struct gateData {
    // Time since we started
    unsigned long millis;
    unsigned long watchdog_count;

    // Power
    uint16_t battery;
    uint16_t solar;

    // Gate Status
    uint8_t gate;
    uint8_t movement;

    // Trigger event
    uint8_t gate_triggered;
    uint8_t movement_triggered;

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
    gate_payload.data.gate = gate;
    gate_payload.data.movement = movement;
    if (trigger == trigger_gate) {
        gate_payload.data.gate_triggered = 0x1;
        gate_payload.data.movement_triggered = 0x0;
    } else {
        gate_payload.data.gate_triggered = 0x0;
        gate_payload.data.movement_triggered = 0x1;
    }
}

#endif
