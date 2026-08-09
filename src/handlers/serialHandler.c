#include "serialHandler.h"
#include "dataHandler.h"
#include <string.h>
#include <stdint.h>

/*
    he was whipping up shit in a file, boiling shit
        actual shit?
    it was boiling poo, in a file. bro the syntax, the syntax
        he was boiling shit?
    in a file bro

*/

bool isOwnMessageSent = false;
bool isOwnMessageReceived = false;

DataFrameData lastDFrame;

// pass pointer to string, length of string and DataType
int writeSerial(char *data, size_t len, DataType type) {
    switch (type) {
        case DATA_TYPE_STR:
            return uart_write_bytes(UART_PORT, data, strlen(data));
        case DATA_TYPE_DATA:
            return uart_write_bytes(UART_PORT, data, len);
        default:
            return -1;
    }
}


int readSerial(char *buf, size_t maxLen) {
    if (buf == NULL || maxLen == 0) return -1;
    int readBytes = uart_read_bytes(UART_PORT, (uint8_t *)buf, maxLen - 1, pdMS_TO_TICKS(10));
    if (readBytes > 0) {
        buf[readBytes] = '\0';
        return readBytes;
    }
    buf[0] = '\0';
    return 0;
}

bool waitForString(const char *expected, uint32_t timeout_ms) { // yo what thte fuck does this function do
    char buffer[BUF_SIZE] = {0};
    size_t buffer_len = 0;
    char chunk[64];
 
    TickType_t start = xTaskGetTickCount();
 
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms)) {
        int n = readSerial(chunk, sizeof(chunk) - 1);
        if (n > 0 && buffer_len + n < sizeof(buffer)) {
            memcpy(buffer + buffer_len, chunk, n);
            buffer_len += n;
            buffer[buffer_len] = '\0';
            // i think i smoked something here
            if (strstr(buffer, expected) != NULL) {
                return true;
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    return false;
} // i think this checks if a string was found within the timeout_ms window and returns false if it wasnt, its a one-shot functiton

void waitForInitAck(void) {
    ESP_LOGI("serialHandler", "Beginning init...");
    while (1) {
        writeSerial("INIT_REQ", strlen("INIT_REQ"), DATA_TYPE_STR);
        if (waitForString("BAS_INIT_ACK", 500)) {
            return;
        }
    }
}

// BAS_ACK is an ack message from the base, while STA_ACK is an ack from the station esp32
bool isSentMessageAcknowledged(void) {
    if (waitForString("BAS_ACK", 500)) {
        return true;
    }
    return false;
}

void sendAck(void) {
    writeSerial("STA_ACK", strlen("STA_ACK"), DATA_TYPE_STR);
}

/*
    data structure to base:
    STA_DAT:AILXXXXX:ELVXXXXX:THRXXX:BATXXXX:STA_EOF
    init, uint16_t, uint16_t, uint8_t, [some type formatted as 12.34v], end of frame
    48 characters
*/

/*
    non data structure to base:
    STA_NDF:XXX:MESSAGE_GOES_HERE:STA_EOF
    XXX is the char length of the message
    constant 20 characters
*/

// todo: maybe recursively send every x ms while isSentMessageAcknowledged is false

bool isDataFrameStreamInterrupt = false;
bool readyToSendDFrame = false;

// out is the array of char instance in sendDataFrame, out_size is its length
void createDataFrame(char *out, size_t out_size, uint16_t ail, uint16_t elv, uint8_t thr, float bat) {
    snprintf(out, out_size, "STA_DAT:AIL%u:ELV%u:THR%u:BAT%u:STA_EOF", ail, elv, thr, bat);
}

void createAndQueueNonDataFrame(const char *str) {
    isDataFrameStreamInterrupt = true;

    char frame[strlen(str) + NDFRAME_BASE_BUFFER];
    snprintf(frame, sizeof(frame), "STA_NDF:XXX:%s:STA_EOF", str);

    writeSerial(frame, strlen(frame), DATA_TYPE_STR);
    isDataFrameStreamInterrupt = false;
}

void sendDataFrame(uint16_t ail, uint16_t elv, uint8_t thr, uint16_t bat) {
    char frame[DFRAME_BUFFER];
    if (!isDataFrameStreamInterrupt) {
        if (waitForString("BAS_EOF", 500)) { // checks if an incoming EOF was sent (checking for ACK might clash with the incoming data as ACK is always before the frame)
            createDataFrame(frame, sizeof(frame), ail, elv, thr, bat);
            writeSerial(frame,  strlen(frame), DATA_TYPE_DATA);
        }
        isDataFrameStreamInterrupt = false;
    } else {
        return;
    }
}

// todo: make an array or something in dataHandler which stores the values sent and received

//splits between readSerial for dframe and ndframe
void readDataFrame(DataFrameData *out) {
    char frame[DFRAME_BUFFER];
    if (isSentMessageAcknowledged()) {
        if (waitForString("BAS_DATA", 500)) {
            readSerial(frame, DFRAME_BUFFER);
            sscanf(frame, "BAS_DAT:AIL%u:ELV%u:THR%u:BAT%u:BAS_EOF", // splits up incoming frame from base
                    &out->ail, 
                    &out->elv, 
                    &out->thr, 
                    &out->bat);
        }
    }
}


void serialTask(void *pvParameters) {
    (void)pvParameters;

    waitForInitAck(); // breaks and continues task once the base's init_ack is received

    while(1) {
        if (isSentMessageAcknowledged()) {
            readDataFrame(&lastDFrame);
            sendAck();
            sendDataFrame(32767, 32767, 128, 1197);
        }



    }  
}