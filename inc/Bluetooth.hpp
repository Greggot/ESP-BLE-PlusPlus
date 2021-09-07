//#include <build.h>
#include "esp_event.h"
#include "esp_netif.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include <vector>
#include "esp_gatts_api.h"

typedef __uint8_t byte;

class Characteristic
{
    private:
        typedef void BLEcallbackType(Characteristic*, esp_ble_gatts_cb_param_t*);

        esp_bt_uuid_t UUID;
        esp_attr_value_t Char_Data;

        esp_gatt_perm_t Permition;
        esp_gatt_char_prop_t Property;

        esp_attr_control_t Control;

        uint16_t Handler;
        esp_gatt_if_t GATT_Interface;
        
        static void DefaultReadCallback(Characteristic*, esp_ble_gatts_cb_param_t*);
        BLEcallbackType* ReadHandler = &DefaultReadCallback;
        static void DefaultWriteCallback(Characteristic*, esp_ble_gatts_cb_param_t*);
        BLEcallbackType* WriteHandler = &DefaultWriteCallback;

        void UUID_Init(uint32_t _UUID);
    public:
        void InfoOut();

        /* SETters */
        void setReadhandler(BLEcallbackType*);
        void setHandler(uint16_t);
        void setGATTinterface(esp_gatt_if_t);
        void setData(byte* Data, size_t Data_Length);
        void setDataMexLength(size_t Data_Max_Length);
        void setPermition(esp_gatt_perm_t);
        void setProperty(esp_gatt_char_prop_t);

        /* GETters */
        esp_gatt_if_t getGATTinterface() { return GATT_Interface; }
        uint16_t getHandler() { return Handler; }
        esp_attr_value_t getData() { return Char_Data; }

        /* CALLers */
        void callReadHandler(esp_ble_gatts_cb_param_t *param);

        esp_err_t AttachToService(uint16_t ServiceHandler);

        void Notify(byte* Data, size_t Data_Length, uint16_t connected_device_id = 0);

        Characteristic();
        Characteristic(uint32_t _UUID);
        Characteristic(uint8_t _UUID[ESP_UUID_LEN_128]);

        Characteristic(uint32_t _UUID, esp_gatt_perm_t, esp_gatt_char_prop_t);
        Characteristic(uint8_t _UUID[ESP_UUID_LEN_128], esp_gatt_perm_t, esp_gatt_char_prop_t);
};

class Service
{
    private:
        esp_bt_uuid_t UUID;
        void UUID_Init(uint32_t _UUID);
    
        esp_gatt_if_t GATTinterface;
        esp_gatt_srvc_id_t service_id;
        uint16_t num_handle;
        uint16_t Handler;

        std::vector<Characteristic*> Characteristics;
    public:
        uint16_t CharCounter = 0;

        void GetInfo();
        /* SETters */
        void setHandler(uint16_t Handler) { this->Handler = Handler; }
        void setGATTinterface(esp_gatt_if_t GATTinterface);
        /* GETters */
        std::vector<Characteristic*> getCharacteristics() { return this->Characteristics; }
        uint16_t getHandler() { return this->Handler; }

        Service();
        Service(uint32_t _UUID, std::vector<Characteristic*> Characteristics);

        void Start();
        void Create();
        void CharacteristicInitialization();
};

class ServerDevice
{
    private:
        esp_ble_adv_data_t adv_data;
        esp_ble_adv_data_t scan_rsp_data;
        esp_ble_adv_params_t adv_params;
        
        esp_gatt_if_t GATT_Interface;

        std::vector<Service*> SERVICE;
        std::vector<int> AmountOfChars;
        uint16_t ServiceInitializeCounter = 0;
        uint16_t CharacteristicInitializeCount = 0;
        uint16_t CharacteristicInitializeCountMax;
        const char* Name;

        void DeviceCallback(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
    public:
        ServerDevice();
        ServerDevice(const char* Name);
        ServerDevice(const char* Name, esp_ble_adv_data_t adv_data, esp_ble_adv_data_t scan_rsp_data, esp_ble_adv_params_t adv_params, std::vector<Service*> SERVICE);

        std::vector<Service*> getServices() { return this->SERVICE; }

        void HandleEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, Service* service) 
            { DeviceCallback(event, gatts_if, param); }

        void Start(esp_gatt_if_t GATT_Interface);
        void AddService(Service* service) { SERVICE.push_back(service); }
};