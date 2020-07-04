#pragma once
#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#define NRF_SDH_DISPATCH_MODEL  NRF_SDH_DISPATCH_MODEL_POLLING

#define NRF_LOG_DEFAULT_LEVEL           (4)
#define NRF_LOG_BACKEND_RTT_ENABLED     (1)
#define NRF_LOG_BACKEND_UART_ENABLED    (0)
#define NRF_LOG_DEFERRED                (0)
#define NRF_LOG_USES_TIMESTAMP          (1)
#define NRF_LOG_STR_FORMATTER_TIMESTAMP_FORMAT_ENABLED  (0)

#define NRF_BLE_QWR_ENABLED             (1)
#define BLE_NUS_ENABLED                 (1)

#define NRF_SDH_BLE_VS_UUID_COUNT       (1)

#define NRF_SDH_BLE_GATT_MAX_MTU_SIZE   151
#define NRF_SDH_BLE_GAP_DATA_LENGTH     155
#define NRF_BLE_CONN_PARAMS_MAX_SLAVE_LATENCY_DEVIATION         65535
#define NRF_BLE_CONN_PARAMS_MAX_SUPERVISION_TIMEOUT_DEVIATION   65535

#define SPI_ENABLED        1
#define NRFX_SPIM_ENABLED  1
#define NRFX_SPIM0_ENABLED 0
#define NRFX_SPIM1_ENABLED 0
#define NRFX_SPIM2_ENABLED 0
#define NRFX_SPIM3_ENABLED 1

#define APP_SDCARD_SPI_INSTANCE 3

#define APP_SDCARD_FREQ_INIT 0x0A000000
#define APP_SDCARD_ENABLED 1

#endif // !__APP_CONFIG_H__
