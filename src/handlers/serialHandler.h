#include <driver/uart.h>
#include <esp_log.h>

#define UART_PORT UART_NUM_1
#define BUF_SIZE 1024


typedef enum  {
    DATA_TYPE_STR,
    DATA_TYPE_DATA
} DataType;

int writeSerial(uint8_t *data, size_t len, DataType type);
int readSerial(uint8_t *buf, size_t max_len);