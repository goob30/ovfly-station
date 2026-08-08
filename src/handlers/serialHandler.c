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

int writeSerial(uint8_t *data, size_t len, DataType type) {
    switch (type) {
        case DATA_TYPE_STR:
            return uart_write_bytes(UART_PORT, (const char*)data, strlen((char *) data));
        case DATA_TYPE_DATA:
            return uart_write_bytes(UART_PORT, (const char *) data, len);
        default:
            return -1;
    }
    return uart_write_bytes(UART_PORT, (const char *)data, len);
}

int readSerial(uint8_t *buf, size_t max_len) {
    return uart_read_bytes(UART_PORT, buf, max_len, pdMS_TO_TICKS(10));
}

bool waitForString(const char *expected, uint32_t timeout_ms) { // yo what thte fuck does this function do
    char buffer[BUF_SIZE] = {0};
    size_t buffer_len = 0;
    uint8_t chunk[64];
 
    TickType_t start = xTaskGetTickCount();
 
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms)) {
        int n = read_serial(chunk, sizeof(chunk) - 1);
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
        writeSerial((uint8_t*)"INIT_REQ", strlen("INIT_REQ"), DATA_TYPE_STR);
        if (waitForString("BAS_INIT_ACK", 500)) {
            return;
        }
    }
}

// BAS_ACK is an ack message from the base, while STA_ACK is an ack from the station esp32
bool isSentMessageAcknowledged(uint8_t *data, size_t len, DataType type) {
    if (!waitForString("BAS_ACK", 500)) {
        ESP_LOGW("serialHandler", "Previous message timing out...");
        if (!waitForString("BAS_ACK", 500)) {
            ESP_LOGW("serialHandler", "Previous message timed out!");
        }
    } else if (waitForString("BAS_ACK", 500)) {
        return true;
    }
    return false;
}


/*
    data structure to base:
    STA_DATA:AILXXXXX:ELVXXXXX:THRXXX:BATXXXX:STA_EOF
    init, uint16_t, uint16_t, uint8_t, [some type formatted as 12.34v], end of frame
    49 characters
*/
// todo: maybe recursively send every x ms while isSentMessageAcknowledged is false

void sendDataFrame(uint8_t *data, size_t len, DataType type) {
    if (waitForString("BAS_EOF", 500)) { // checks if an incoming EOF was sent (checking for ACK might clash with the incoming data as ACK is always before the frame)
        writeSerial("STA_ACK", strlen("STA_ACK"), DATA_TYPE_STR);
        writeSerial("STA_DATA:AIL32767:ELV32767:THR128:BAT1168:STA_EOF", 49, DATA_TYPE_DATA);
    }
}



void serialTask(void *pvParameters) {
    (void)pvParameters;

    waitForInitAck(); // breaks and continues task once the base's init_ack is received

    
    
}