#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/twai.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "esp_log.h"

#include "lwip/sockets.h"

#include "nvs_flash.h"
#include "sdkconfig.h"

#define BLINK_GPIO GPIO_NUM_25
#define BUTTON_GPIO GPIO_NUM_0

#define PORT 3333

///          FUNCTION Definitions           ///
extern "C" { void app_main(void); }
///     WiFi module     ///
void WiFi_Init_AP(const char* WiFi_SSID, uint8_t WiFi_SSID_Length, const char* WiFi_password, uint8_t WiFi_password_length, uint8_t WiFi_MaxConnectionNumber);

///     Socket module   ///
void Socket_Thread(void* pvParameter);

///     CAN module     ///
void CAN_Init(twai_timing_config_t baudrate);
void CAN_RX_Callback(twai_message_t message);   /// User defined callback
void CAN_Pause();
void CAN_Play();
bool isCANenabled();