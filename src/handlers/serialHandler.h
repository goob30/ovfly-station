#pragma once
#include <driver/uart.h>
#include <esp_log.h>
#include "dataHandler.h"


#define UART_PORT UART_NUM_0
#define BUF_SIZE 1024
#define DFRAME_BUFFER 64
#define NDFRAME_BASE_BUFFER 20



typedef enum  {
    DATA_TYPE_STR,
    DATA_TYPE_DATA
} DataType;

void initUart();

int writeSerial(char *data, size_t len, DataType type);
bool waitForString(const char *expected, uint32_t timeout_ms);
int readSerial(char *buf, size_t maxLen);

void waitForInitAck(void);
void sendDataFrame(uint16_t ail, uint16_t elv, uint8_t thr, uint16_t bat);
void createAndQueueNonDataFrame(const char *str);

void serialTask(void *pvParameters);