#include <FreeRTOSConfig.h>
#include "handlers/serialHandler.h"
#include "handlers/dataHandler.h"


void app_main() {
    initUart();
    xTaskCreate(serialTask, "serialTask", 4096, NULL, 5, NULL);
    xTaskCreate(dataTask, "dataTask", 4096, NULL, 5, NULL);
}