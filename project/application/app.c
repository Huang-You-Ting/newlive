#include "nrf_log.h"
#include "ble_nus.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <string.h>

static SemaphoreHandle_t prvMutex;
static SemaphoreHandle_t prvSemi;

static char prvFileName[256];

static bool bBleNotifyIsEnable = false;

uint8_t pseudoData[BLE_NUS_MAX_DATA_LEN];
static void prvAppTask(void* pArg) {
    for(;;) {
        xSemaphoreTake(prvSemi, portMAX_DELAY);  
        TickType_t t0 = xTaskGetTickCount();
        for(int i=0; i<10000  ;i++)
        {
            memset(pseudoData, i%0xFF, sizeof(pseudoData));
            extern void vUpdateData(void *pvSrc, uint32_t ulSize);
            if (bBleNotifyIsEnable) {
               vUpdateData(pseudoData, sizeof(pseudoData));
            }
        }
        NRF_LOG_INFO("send %d bytes, take %ld ms. ( %d kbps)", 
            BLE_NUS_MAX_DATA_LEN * 10000, 
            xTaskGetTickCount() -t0,
            ((BLE_NUS_MAX_DATA_LEN * 10000)*1000)/((xTaskGetTickCount() - t0)*1024) * 8
            );       
    }
    
}

void vAppInit(void) {
    prvSemi = xSemaphoreCreateBinary();
    xTaskCreate(prvAppTask, "app", 1024, NULL, 3, NULL);
}

void vAppFileNameSet(void *pvSrc, uint32_t ulSize) {    
    memcpy(prvFileName, pvSrc, ulSize);
    prvFileName[ulSize] = '\0';
    NRF_LOG_INFO("file name: %s", prvFileName);       
    xSemaphoreGive(prvSemi);
}

void vAppNotifyEnble(void) {
    bBleNotifyIsEnable = true;
}

void vAppNotifyDisble(void) {
    bBleNotifyIsEnable = false;
}