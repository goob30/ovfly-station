#include "serialHandler.h"
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

bool waitForString(const char *expected, uint32_t timeout_ms) {
    char acc[BUF_SIZE] = {0};
    size_t acc_len = 0;
    uint8_t chunk[64];
 
    TickType_t start = xTaskGetTickCount();
 
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms)) {
        int n = read_serial(chunk, sizeof(chunk) - 1);
        if (n > 0 && acc_len + n < sizeof(acc)) {
            memcpy(acc + acc_len, chunk, n);
            acc_len += n;
            acc[acc_len] = '\0';
 
            if (strstr(acc, expected) != NULL) {
                return true;
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    return false;
}

void waitForInitAck() {
    ESP_LOGI("serialHandler", "Beginning init...");
    while (1) {
        writeSerial((uint8_t*)"INITREQ", strlen("INITREQ"), DATA_TYPE_STR);
        if (waitForString("INITACK", 500)) {
            return;
        }
    }
}

bool sendAndWait(uint8_t *data, size_t len, DataType type) {
    if (!isOwnMessageSent) {
        isOwnMessageSent = true;
    } else {
        if (!waitForString("INC_ACK", 500)) {
            ESP_LOGW("serialHandler", "Previous message timing out...");
            if (!waitForString("INC_ACK", 500)) {
                ESP_LOGW("serialHandler", "Previous message timed out!");
            }
        }
    }
}
