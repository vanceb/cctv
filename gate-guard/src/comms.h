#ifndef COMMS_H
#define COMMS_H

#include "Arduino.h"

#include "data.h"
#include <XBee.h>

#define XBEE_SERIAL Serial
#define XBEE_BAUD 9600
#define COORDINATOR_SH 0x0
#define COORDINATOR_SL 0x0

#define XBEE_SLEEP 7 // Setting Pin 7 high sends the XBee to PinSleep mode (Pin 9 on the XBee)
#define CTS 6 // CTS indicates that the XBee is good to receive data
#define TX_CTS_TIMEOUT 1000
#define TX_RESPONSE_TIMEOUT 5000

#define XBEE_SLEEP_CYCLES 0 // Not fully implemented yet, so set to 0 for wake every time
int xbeeSleepCount = 0;

#define TELEMETRY_APP_ID 0x0573
#define TX_STATUS_MSG 0x0001
#define TX_STATUS_VERSION 0x0001
#define TX_STATUS_UPDATE_PERIOD 60000

uint32_t nextTxUpdate;

// Define a set of counters to retain information about the TX status
static uint32_t  tx_packets = 0;
static uint16_t tx_local_cts_timeout = 0;
static uint16_t tx_local_timeout = 0;
static uint16_t tx_cca_failure = 0;
static uint16_t tx_invalid_dest = 0;
static uint16_t tx_network_ack = 0;
static uint16_t tx_not_joined = 0;
static uint16_t tx_self_addr = 0;
static uint16_t tx_addr_not_found = 0;
static uint16_t tx_no_route = 0;
static uint16_t tx_payload_too_big = 0;
static uint16_t tx_other1 = 0;
static uint16_t tx_other2 = 0;

// Define a structure for the tx data
typedef struct tagTxStatus {
  // Time since restart
  uint32_t millis;
  // Tx Failures
  uint16_t tx_packets;
  uint16_t tx_local_cts_timeout;
  uint16_t tx_local_timeout;
  uint16_t tx_cca_failure;
  uint16_t tx_invalid_dest;
  uint16_t tx_network_ack;
  uint16_t tx_not_joined;
  uint16_t tx_self_addr;
  uint16_t tx_addr_not_found;
  uint16_t tx_no_route;
  uint16_t tx_payload_too_big;
  uint16_t tx_other1;
  uint16_t tx_other2;
} TxStatus;

typedef struct tagXbeeTxStatusCommBuffer {

    // Header data
    uint16_t appID;
    uint16_t msgType;
    uint16_t version;

    // Data length
    uint16_t length; //could be just a byte, but keeping it on a 16 bit boundary helps decoding

    // The data itself
    TxStatus data;

} TxStatusPayload;

// Create a global istance of the Tx PStatus Payload which we will fill usng the function below
TxStatusPayload txStatusPayload;

void fillTxStatus(){
    txStatusPayload.appID = TELEMETRY_APP_ID;
    txStatusPayload.msgType = TX_STATUS_MSG;
    txStatusPayload.version = TX_STATUS_VERSION;
    txStatusPayload.length = sizeof(TxStatus);
    txStatusPayload.data.millis = millis();
    txStatusPayload.data.tx_packets = tx_packets;
    txStatusPayload.data.tx_local_cts_timeout = tx_local_cts_timeout;
    txStatusPayload.data.tx_local_timeout = tx_local_timeout;
    txStatusPayload.data.tx_cca_failure = tx_cca_failure;
    txStatusPayload.data.tx_invalid_dest = tx_invalid_dest;
    txStatusPayload.data.tx_network_ack = tx_network_ack;
    txStatusPayload.data.tx_not_joined = tx_not_joined;
    txStatusPayload.data.tx_self_addr = tx_self_addr;
    txStatusPayload.data.tx_addr_not_found = tx_addr_not_found;
    txStatusPayload.data.tx_no_route = tx_no_route;
    txStatusPayload.data.tx_payload_too_big = tx_payload_too_big;
    txStatusPayload.data.tx_other1 = tx_other1;
    txStatusPayload.data.tx_other2 = tx_other2;
}

// Create a global reference to the xbee interface and other variables
static XBee xbee = XBee();
static XBeeAddress64 addr64 = XBeeAddress64(COORDINATOR_SH, COORDINATOR_SL);
static ZBTxStatusResponse txStatus = ZBTxStatusResponse();
static ZBTxRequest zbTxGt = ZBTxRequest(addr64, (uint8_t *)&gate_payload, sizeof(gate_payload));
static ZBTxRequest zbTxStatus = ZBTxRequest(addr64, (uint8_t *)&txStatusPayload, sizeof(txStatusPayload));

// Initialize the xbee - to be called in setup()
void xbeeInit(){
    // Make sure the XBee is awake
    pinMode(XBEE_SLEEP, OUTPUT);
    digitalWrite(XBEE_SLEEP, LOW);
    // Setup CTS Pin
    pinMode(CTS, INPUT);

   // Initialize the comms
    XBEE_SERIAL.begin(XBEE_BAUD);
    xbee.begin(XBEE_SERIAL);

    // Setup TX Status Updates
    nextTxUpdate = millis() + TX_STATUS_UPDATE_PERIOD;
}

void sleepXBee(){
    // Signal to turn off the XBee
    pinMode(XBEE_SLEEP, OUTPUT);
    digitalWrite(XBEE_SLEEP, HIGH);
}

// Returns the number of calls to this function before the XBee will actually wake - 0 means we woke it
void wakeXBee(){
    pinMode (XBEE_SLEEP, OUTPUT);
    digitalWrite(XBEE_SLEEP, LOW);
}

// Send data using the XBee - should return 0 if success
static int xbeeSend(ZBTxRequest zbTx){
   tx_packets++;
   // Make sure the XBee is awake
    wakeXBee();
    // Wait for CTS from the XBee or timeout
    // Using CTS required a hardware modification to the Seeedstudio Stalker
    // Pin 6 (presented at JP6 has been connected to the CTS pin of the XBee
    // using a jumper wire Soldered to the jumper JP6 on the back of the board
    bool gotCTS = false;
    unsigned long timeout = millis() + TX_CTS_TIMEOUT;
    while(millis() < timeout) {
      if (digitalRead(CTS) == false) {
        gotCTS = true;
        break;
      } else {
        delay(10);
      }
    }

    if(gotCTS == true){
    // Send the Sensor data API frame to the Xbee
      xbee.getNextFrameId();
      xbee.send(zbTx);
      if(xbee.readPacket(TX_RESPONSE_TIMEOUT)){
          // We got a response from our local xbee
          if(xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE){
              // This is the "all OK" route...
              xbee.getResponse().getZBTxStatusResponse(txStatus);
              // Update failure counters if necessary
              switch(txStatus.getDeliveryStatus()){
                case SUCCESS :
                  // Most likely route so get it out of the way first
                  // See whether we should send a TX Status report
                  if (millis() > nextTxUpdate) {

                        fillTxStatus();
                        xbee.getNextFrameId();
                        xbee.send(zbTxStatus);
                        if(xbee.readPacket(TX_RESPONSE_TIMEOUT)){
                          // We got a response from our local xbee
                          if(xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE){
                            // This is the "all OK" route...
                            xbee.getResponse().getZBTxStatusResponse(txStatus);
                            if(txStatus.getDeliveryStatus() == SUCCESS) {
                              nextTxUpdate = millis() + TX_STATUS_UPDATE_PERIOD;
                            }
                         }
                      }
                  }
                  return 0;
                  break;
                case CCA_FAILURE :
                  tx_cca_failure++;
                  return -5;
                  break;
                case INVALID_DESTINATION_ENDPOINT_SUCCESS :
                  tx_invalid_dest++;
                  return -6;
                  break;
                case NETWORK_ACK_FAILURE :
                  tx_network_ack++;
                  return -3;
                  break;
                case NOT_JOINED_TO_NETWORK :
                  tx_not_joined++;
                  return -4;
                  break;
                case SELF_ADDRESSED :
                  tx_self_addr++;
                  return -7;
                  break;
                case ADDRESS_NOT_FOUND :
                  tx_addr_not_found++;
                  return -8;
                  break;
                case ROUTE_NOT_FOUND :
                  tx_no_route++;
                  return -9;
                  break;
                case PAYLOAD_TOO_LARGE :
                  tx_payload_too_big++;
                  return -10;
                  break;
                default :
                  // Unexpected error
                  tx_other1++;
                  return -11;
              }
              return txStatus.getDeliveryStatus();
          } else {
            // There was another recieve packet or modem status???
            // Increment the failure counter
            tx_other2++;
            return -12;
          }
      } else {
        //Timed out waiting for a response from the XBee about the TX Status
        // Increment the failure counter
        tx_local_timeout++;
        return -2;
      }
    } else {
      // We didn't get CTS from the local XBee within the timeout
      // Update the failure counter
      tx_local_cts_timeout++;
      return -1;
    }
}

#endif
