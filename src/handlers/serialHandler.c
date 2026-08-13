#include "serialHandler.h"
#include "dataHandler.h"
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

void initUart(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_PORT, &uart_config);
    uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
}

DataFrameData lastDFrame;
int lastBasAckTick;

int writeSerial(char *data, size_t len, DataType type) {
    switch (type) {
        case DATA_TYPE_STR:  return uart_write_bytes(UART_PORT, data, strlen(data));
        case DATA_TYPE_DATA: return uart_write_bytes(UART_PORT, data, len);
        default: return -1;
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

static char rxBuffer[BUF_SIZE] = {0};
static size_t rxBufferLen = 0;

static void FillRxBuffer(void) {
    char chunk[64];
    int n = readSerial(chunk, sizeof(chunk) - 1);
    if (n > 0 && rxBufferLen + (size_t)n < sizeof(rxBuffer) - 1) {
        memcpy(rxBuffer + rxBufferLen, chunk, n);
        rxBufferLen += n;
        rxBuffer[rxBufferLen] = '\0';
    }
}

bool waitForString(const char *expected, uint32_t timeout_ms) {
    TickType_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms)) {
        FillRxBuffer();
        char *pos = strstr(rxBuffer, expected);
        if (pos != NULL) {
            size_t matchEnd = (pos - rxBuffer) + strlen(expected);
            memmove(rxBuffer, rxBuffer + matchEnd, rxBufferLen - matchEnd + 1);
            rxBufferLen -= matchEnd;
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return false;
}

static bool TryParseFrame(DataFrameData *out) {
    char *startPos = strstr(rxBuffer, "BAS_DAT");
    if (!startPos) {
        if (rxBufferLen > DFRAME_BUFFER * 2) {
            rxBufferLen = 0;
            rxBuffer[0] = '\0';
        }
        return false;
    }

    char *endPos = strstr(startPos, "BAS_EOF");
    if (!endPos) return false; // frame start seen, not complete yet

    size_t frameEnd = (endPos - rxBuffer) + strlen("BAS_EOF");
    size_t frameLen = frameEnd - (startPos - rxBuffer);

    char frame[DFRAME_BUFFER];
    size_t copyLen = frameLen < sizeof(frame) - 1 ? frameLen : sizeof(frame) - 1;
    memcpy(frame, startPos, copyLen);
    frame[copyLen] = '\0';

    int matched = sscanf(
        frame, "BAS_DAT:AIL%" SCNu16 ":ELV%" SCNu16 ":THR%" SCNu8 ":BAT%" SCNu16 ":BAS_EOF",
        &out->ail, &out->elv, &out->thr, &out->bat
    );

    memmove(rxBuffer, rxBuffer + frameEnd, rxBufferLen - frameEnd + 1);
    rxBufferLen -= frameEnd;

    return matched == 4;
}

void waitForInitAck(void) {
    ESP_LOGI("serialHandler", "Beginning init...");
    while (1) {
        writeSerial("INIT_REQ", strlen("INIT_REQ"), DATA_TYPE_STR);
        if (waitForString("BAS_INIT_ACK", 500)) {
            lastBasAckTick = xTaskGetTickCount();
            return;
        }
    }
}

void sendAck(void) {
    writeSerial("STA_ACK", strlen("STA_ACK"), DATA_TYPE_STR);
}

void createDataFrame(char *out, size_t out_size, uint16_t ail, uint16_t elv, uint8_t thr, uint16_t bat) {
    snprintf(out, out_size, "STA_DAT:AIL%" PRIu16 ":ELV%" PRIu16 ":THR%" PRIu8 ":BAT%" PRIu16 ":STA_EOF", ail, elv, thr, bat);
}

void createAndQueueNonDataFrame(const char *str) {
    char frame[strlen(str) + NDFRAME_BASE_BUFFER];
    snprintf(frame, sizeof(frame), "STA_NDF:XXX:%s:STA_EOF", str);
    writeSerial(frame, strlen(frame), DATA_TYPE_STR);
}

void sendDataFrame(uint16_t ail, uint16_t elv, uint8_t thr, uint16_t bat) {
    char frame[DFRAME_BUFFER];
    createDataFrame(frame, sizeof(frame), ail, elv, thr, bat);
    writeSerial(frame, strlen(frame), DATA_TYPE_DATA);
}

bool isSerialConnectionActive(void) {
    return (xTaskGetTickCount() - lastBasAckTick) < pdMS_TO_TICKS(2000);
}

void serialTask(void *pvParameters) {
    (void)pvParameters;

    waitForInitAck();

    while (1) {
        FillRxBuffer();

        char *ackPos = strstr(rxBuffer, "BAS_ACK");
        if (ackPos) {
            size_t end = (ackPos - rxBuffer) + strlen("BAS_ACK");
            memmove(rxBuffer, rxBuffer + end, rxBufferLen - end + 1);
            rxBufferLen -= end;
            lastBasAckTick = xTaskGetTickCount();
        }

        DataFrameData parsed;
        if (TryParseFrame(&parsed)) {
            lastDFrame = parsed;
        }

        sendAck();
        sendDataFrame(32767, 32767, 128, 1197);

        if (!isSerialConnectionActive()) {
            waitForInitAck();
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}