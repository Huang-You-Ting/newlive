#include "nrf.h"
#include "bsp.h"
#include "ff.h"
#include "diskio_blkdev.h"
#include "nrf_block_dev_sdc.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "ble_nus.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stream_buffer.h"

#include <string.h>

#define FILE_NAME   "NORDIC.TXT"
#define TEST_STRING "SD card example. abcdefghijklmnopqrstuvwxyz this is a book"

#define SDC_SCK_PIN     NRF_GPIO_PIN_MAP(1, 15)  ///< SDC serial clock (SCK) pin.
#define SDC_MOSI_PIN    NRF_GPIO_PIN_MAP(1, 13)  ///< SDC serial data in (DI) pin.
#define SDC_MISO_PIN    NRF_GPIO_PIN_MAP(1, 14)  ///< SDC serial data out (DO) pin.
#define SDC_CS_PIN      NRF_GPIO_PIN_MAP(1, 12)  ///< SDC chip select (CS) pin.

NRF_BLOCK_DEV_SDC_DEFINE(
        m_block_dev_sdc,
    NRF_BLOCK_DEV_SDC_CONFIG(
            SDC_SECTOR_SIZE,
        APP_SDCARD_CONFIG(SDC_MOSI_PIN, SDC_MISO_PIN, SDC_SCK_PIN, SDC_CS_PIN)),
    NFR_BLOCK_DEV_INFO_CONFIG("Nordic", "SDC", "1.00"));



static SemaphoreHandle_t prvMutex;
static SemaphoreHandle_t prvSemi;
static StreamBufferHandle_t prvSendStream;

static char prvFileName[256];

static bool bBleNotifyIsEnable = false;

uint8_t pseudoData[BLE_NUS_MAX_DATA_LEN];
static FATFS fs;
static DIR dir;
static FILINFO fno;
static FIL file;
static void prvFatfsInit(void)
{


    uint32_t bytes_written;
    FRESULT ff_result;
    DSTATUS disk_state = STA_NOINIT;

    // Initialize FATFS disk I/O interface by providing the block device.
    static diskio_blkdev_t drives[] =
    {
        DISKIO_BLOCKDEV_CONFIG(NRF_BLOCKDEV_BASE_ADDR(m_block_dev_sdc, block_dev), NULL)
    };

    diskio_blockdev_register(drives, ARRAY_SIZE(drives));

    NRF_LOG_INFO("Initializing disk 0 (SDC)...");
    for (uint32_t retries = 3; retries && disk_state; --retries)
    {
        disk_state = disk_initialize(0);
    }
    if (disk_state)
    {
        NRF_LOG_INFO("Disk initialization failed.");
        return;
    }

    uint32_t blocks_per_mb = (1024uL * 1024uL) / m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_size;
    uint32_t capacity = m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_count / blocks_per_mb;
    NRF_LOG_INFO("Capacity: %d MB", capacity);

    NRF_LOG_INFO("Mounting volume...");
    ff_result = f_mount(&fs, "", 1);
    if (ff_result)
    {
        NRF_LOG_INFO("Mount failed.");
        return;
    }

    NRF_LOG_INFO("\r\n Listing directory: /");
    ff_result = f_opendir(&dir, "/");
    if (ff_result)
    {
        NRF_LOG_INFO("Directory listing failed!");
        return;
    }

    do
    {
        ff_result = f_readdir(&dir, &fno);
        if (ff_result != FR_OK)
        {
            NRF_LOG_INFO("Directory read failed.");
            return;
        }

        if (fno.fname[0])
        {
            if (fno.fattrib & AM_DIR)
            {
                NRF_LOG_RAW_INFO("   <DIR>   %s", (uint32_t)fno.fname);
            }
            else
            {
                NRF_LOG_RAW_INFO("%9lu  %s", fno.fsize, (uint32_t)fno.fname);
            }
        }
    } while (fno.fname[0]);
    NRF_LOG_RAW_INFO("");

    ff_result = f_stat(FILE_NAME, &fno);
    if (ff_result != FR_OK) {
        NRF_LOG_INFO("Writing to file " FILE_NAME "...");
        ff_result = f_open(&file, FILE_NAME, FA_READ | FA_WRITE | FA_OPEN_APPEND);
        if (ff_result != FR_OK) {
            NRF_LOG_INFO("Unable to open or create file: " FILE_NAME ".");
            return;
        }

        ff_result = f_write(&file, TEST_STRING, sizeof(TEST_STRING) - 1, (UINT *) &bytes_written);
        if (ff_result != FR_OK) {
            NRF_LOG_INFO("Write failed\r\n.");
        } else {
            NRF_LOG_INFO("%d bytes written.", bytes_written);
        }

        (void) f_close(&file);
    }
    return;
}

static uint32_t prvExpectSend; 
static uint32_t prvActualSend; 
static TickType_t t0;
static void prvAppTask(void* pArg) {
    NRF_LOG_INFO("FATFS example started.");
    prvFatfsInit();

    for(;;) {   
        FRESULT ff_result;    
        
        xSemaphoreTake(prvSemi, portMAX_DELAY);   
        
        t0 = xTaskGetTickCount();
        
        ff_result = f_stat(prvFileName, &fno);
        if (ff_result != FR_OK) {
            NRF_LOG_INFO("Unable to open or create file: %s.", prvFileName);
            continue;
        } else {
            NRF_LOG_INFO("file: %s, size: %d byets", prvFileName, fno.fsize);
        }
        
        
        ff_result = f_open(&file, prvFileName, FA_READ);
        if (ff_result != FR_OK)
        {
            NRF_LOG_INFO("Unable to open or create file: %s.", prvFileName);
            continue;
        }

        uint32_t ulExpectSend = fno.fsize;
        prvExpectSend = fno.fsize;
        prvActualSend = 0;
        t0 = xTaskGetTickCount();
        while (ulExpectSend) {
            uint32_t bytes_read = 0;
            ff_result = f_read(&file,
                pseudoData,
                sizeof(pseudoData), 
                (UINT *) &bytes_read);      
            if (ff_result != FR_OK) {
                NRF_LOG_INFO("Read failed\r\n.");
                __BKPT(255);
            } else {
                NRF_LOG_INFO("%d bytes read.", bytes_read);
            }
            ulExpectSend -= bytes_read;
            xStreamBufferSend(prvSendStream, pseudoData, bytes_read, portMAX_DELAY);
        }

        (void) f_close(&file);

    }
    
}



static void prvNotifyTask(void* pArg) {
    for (;;) {
        static uint8_t prvBuf[BLE_NUS_MAX_DATA_LEN];
        uint32_t bytes_read = xStreamBufferReceive(prvSendStream,
            prvBuf,
            sizeof(prvBuf),
            portMAX_DELAY);
        extern void vUpdateData(void *pvSrc, uint32_t ulSize);
        if (bBleNotifyIsEnable && bytes_read > 0) {
            vUpdateData(prvBuf, bytes_read);		 
            prvActualSend += bytes_read;
            if (prvActualSend == prvExpectSend) {
                NRF_LOG_INFO("send %d bytes, take %ld ms. ( %d kbps)", 
                    fno.fsize, 
                    xTaskGetTickCount() - t0,
                    ((fno.fsize) * 1000)/((xTaskGetTickCount() - t0) * 1024) * 8);   
            }
        }
    }
}
void vAppInit(void) {
    prvSemi = xSemaphoreCreateBinary();
    prvSendStream = xStreamBufferCreate(BLE_NUS_MAX_DATA_LEN * 12, 1);
    xTaskCreate(prvAppTask, "app", 4096, NULL, 3, NULL);
    xTaskCreate(prvNotifyTask, "notify", 2048, NULL, 3, NULL);
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
