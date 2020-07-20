#include "nrf.h"
#include "bsp.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "ble_nus.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stream_buffer.h"

#include <string.h>




static SemaphoreHandle_t prvMutex;
static SemaphoreHandle_t prvSemi;
static StreamBufferHandle_t prvSendStream;
static StreamBufferHandle_t prvRecvStream;

static char prvFileName[256];

static bool bBleNotifyIsEnable = false;

uint8_t pseudoData[BLE_NUS_MAX_DATA_LEN];

static uint32_t prvExpectSend; 
static uint32_t prvActualSend; 
static TickType_t t0;
static void prvAppTask(void* pArg) {
    NRF_LOG_INFO("Application Started.");

    for(;;) {   
        static uint8_t prvBuf[BLE_NUS_MAX_DATA_LEN];
        uint32_t bytes_read, bytes_write;
        bytes_read = xStreamBufferReceive(prvRecvStream, prvBuf, sizeof(prvBuf), portMAX_DELAY);        
        bytes_write = xStreamBufferSend(prvSendStream, prvBuf, bytes_read, portMAX_DELAY);
    }
    
}

static void prvNotifyTask(void* pArg) {
    for (;;) {
        static uint8_t prvBuf[BLE_NUS_MAX_DATA_LEN];
        uint32_t bytes_read = xStreamBufferReceive(prvSendStream, prvBuf, sizeof(prvBuf), portMAX_DELAY);
        extern void vUpdateData(void *pvSrc, uint32_t ulSize);
        if (bBleNotifyIsEnable && bytes_read > 0) {
            vUpdateData(prvBuf, bytes_read);		 
        }
    }
}

void vAppInit(void) {
    prvSendStream = xStreamBufferCreate(BLE_NUS_MAX_DATA_LEN * 12, 1);
    prvRecvStream = xStreamBufferCreate(1024, 1);
    xTaskCreate(prvAppTask, "app", 1024, NULL, 3, NULL);
    xTaskCreate(prvNotifyTask, "notify", 1024, NULL, 3, NULL);
}

void vAppSendAvaliable(void) {
    xSemaphoreGive(prvSemi);
}

void vAppReceiveCb(void *pvSrc, uint32_t ulSize) {    
    xStreamBufferSend(prvRecvStream, pvSrc, ulSize, 0);
}

void vAppNotifyEnble(void) {
    bBleNotifyIsEnable = true;
}

void vAppNotifyDisble(void) {
    bBleNotifyIsEnable = false;
}
