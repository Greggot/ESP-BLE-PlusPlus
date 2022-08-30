#pragma once
#include <esp_bt.h>
#include <esp_bt_defs.h>
#include <esp_bt_main.h>
#include <esp_gatt_common_api.h>
#include <esp_gatts_api.h>

#include <cstring>

#include <Bluetooth.hpp>

namespace Bluetooth
{
    extern esp_gatt_if_t GATTinterface;
    class GATTinstance
    {
    protected:
        esp_bt_uuid_t UUID;
        uint16_t Handler;
    public:
        GATTinstance(const uint32_t ID)
        {
            UUID.len = ID & 0xFFFF0000 ? sizeof(uint32_t) : sizeof(uint16_t);
            memcpy(&UUID.uuid, &ID, UUID.len);
        }
        GATTinstance(uint8_t ID[ESP_UUID_LEN_128])
        {
            UUID.len = ESP_UUID_LEN_128;
            memcpy(&UUID.uuid, ID, UUID.len);
        }
        
        #if defined BLE_SERVICES_PRINTF || defined BLE_CHARACTERISTICS_PRINTF
        void ConsoleInfoOut()
        {
            printf("UUID: ");
            for(uint8_t i = 0; i < UUID.len; ++i)
                printf("%02X", UUID.uuid.uuid128[i]);
            printf(", Handler: %d\n", this->Handler);
        }
        #endif
        
        uint16_t getHandler() { return Handler; }
        void setHandler(uint16_t Handler) { this->Handler = Handler; }
    };
}