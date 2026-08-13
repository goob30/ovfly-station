#include <FreeRTOSConfig.h>
#include "handlers/serialHandler.h"
#include "handlers/dataHandler.h"
#include "stationDebug/servoHandler.h"


void app_main() {
    initUart();
    initServos();
    xTaskCreate(serialTask, "serialTask", 4096, NULL, 5, NULL);
    // xTaskCreate(dataTask, "dataTask", 4096, NULL, 5, NULL);
    xTaskCreate(servoTask, "servoTask", 4096, NULL, 5, NULL);
}