#pragma once
#include "esp_event.h"

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"

#include "esp_gap_ble_api.h"

#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"

#include <vector>
#include <map>

#define BLE_SERVICES_PRINTF
#define BLE_CHARACTERISTICS_PRINTF
#define BLE_INPUT_PRINTF

typedef __uint8_t byte;

class GATTinstance
{
    protected:
        esp_bt_uuid_t UUID;
        esp_gatt_if_t GATTinterface;
        uint16_t Handler;
    public:
        #if defined BLE_SERVICES_PRINTF || defined BLE_CHARACTERISTICS_PRINTF
        void ConsoleInfoOut()
        {
            printf("UUID: ");
            if(this->UUID.len == ESP_UUID_LEN_16)
                printf("%04X, ", this->UUID.uuid.uuid16);
            else if(this->UUID.len == ESP_UUID_LEN_32)
                printf("%08X, ", this->UUID.uuid.uuid32);

            printf("Handler: %d\n", this->Handler);
        }
        #endif
        
        uint16_t getHandler() { return this->Handler; }
        void setHandler(uint16_t Handler) { this->Handler = Handler; }
        
        esp_gatt_if_t getGATTinterface() { return this->GATTinterface; }
        void setGATTinterface(esp_gatt_if_t GATTinterface) { this->GATTinterface = GATTinterface; }

};

class Characteristic : public GATTinstance
{
    private:
        typedef void GATTScallbackType(Characteristic*, esp_ble_gatts_cb_param_t*);

        esp_attr_value_t  Char_Data;
        esp_gatt_perm_t Permition;
        esp_gatt_char_prop_t Property;

        bool DataAllocatedInsideObject = false;
        void* Data;
        size_t DataSize;
        
        static void DefaultReadCallback(Characteristic*, esp_ble_gatts_cb_param_t*);
        GATTScallbackType* ReadHandler = &DefaultReadCallback;
        static void DefaultWriteCallback(Characteristic*, esp_ble_gatts_cb_param_t*);
        GATTScallbackType* WriteHandler = &DefaultWriteCallback;

    public:
        void setReadCallback(GATTScallbackType* callback) { this->ReadHandler = callback; }
        void setWriteCallback(GATTScallbackType* callback) { this->WriteHandler = callback; }
        void callReadCallback(esp_ble_gatts_cb_param_t *param);
        void callWriteCallback(esp_ble_gatts_cb_param_t *param);

        void setData(const byte* Data, size_t DataSize);
        void setDynamicData(void* Data, size_t DataSize);
        void* getData() { return this->Data; }
        size_t getDataSize() { return this->DataSize; }

        esp_err_t AttachToService(uint16_t ServiceHandler);

        void Notify(const void* Data, size_t DataSize, uint16_t connected_device_id = 0);
        void Responce(const void* Data, size_t DataSize, esp_ble_gatts_cb_param_t* Param);

        Characteristic(uint32_t _UUID, esp_gatt_perm_t, esp_gatt_char_prop_t);
        Characteristic(uint8_t _UUID[ESP_UUID_LEN_128], esp_gatt_perm_t, esp_gatt_char_prop_t);
};

class Service : public GATTinstance
{
    private:
        esp_gatt_srvc_id_t service_id;
        
        uint16_t num_handle;
        std::vector<Characteristic*> Characteristics;
        size_t CharacteristicsSize;
    public:
        uint16_t CharCounter = 0;

        void setGATTinterface(esp_gatt_if_t GATTinterface);

        std::vector<Characteristic*> getCharacteristics() { return this->Characteristics; }
        size_t getCharacteristicsSize() { return this->CharacteristicsSize; }

        Service(uint32_t _UUID, std::vector<Characteristic*> Characteristics);

        void Start();
        void Create();
};

class ServerDevice
{
    private:
        static const uint8_t MaxEventNumber = 24;
        typedef void GATTScallbackType(ServerDevice*, esp_ble_gatts_cb_param_t*);

        const char* Name;
        esp_ble_adv_data_t AdvertisingData;
        esp_ble_adv_data_t ScanResponceData;
        esp_ble_adv_params_t AdvertisingParameters;
        esp_gatt_if_t GATTinterface;

        std::vector<Service*> Services;
        
        GATTScallbackType* DeviceCallbacks[MaxEventNumber] {NULL};
        void HandleGATTSevents(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

        void Start(esp_gatt_if_t GATTinterface);
    public:
        ServerDevice();
        ServerDevice(const char* Name, esp_ble_adv_data_t AdvertisingData, esp_ble_adv_data_t ScanResponceData, 
                    esp_ble_adv_params_t AdvertisingParameters, std::vector<Service*> Services);

        void setGATTSevent(esp_gatts_cb_event_t Event, GATTScallbackType* Callback);
        
        void HandleEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param) 
            { HandleGATTSevents(event, gatts_if, param); }
};