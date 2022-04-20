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
#include <cstring>

#define BLE_SERVICES_PRINTF
#define BLE_CHARACTERISTICS_PRINTF
#define BLE_INPUT_PRINTF

#ifdef BLE_INPUT_PRINTF
    #define MAX_OUTPUT_AMOUNT 40
#endif

typedef __uint8_t byte;

namespace Perm
{
    enum bit
    {
        Read            = 1,
        ReadEncrypted   = (1 << 1),
        ReadEncrMitm    = (1 << 2),
        Write           = (1 << 4),
        WriteEncrypted  = (1 << 5),
        WriteEncrMitm   = (1 << 6),
        WriteSigned     = (1 << 7),
        WriteSignMitm   = (1 << 8),
        ReadAuth        = (1 << 9),
        WriteAuth       = (1 << 10),
    };
}
namespace Prop
{
    enum bit
    {
        Broadcast   = 1,
        Read        = (1 << 1),
        WriteNr     = (1 << 2),
        Write       = (1 << 3),
        Notify      = (1 << 4),
        Indicate    = (1 << 5),
        Auth        = (1 << 6),
        ExtProp     = (1 << 7),
    };
}

extern esp_gatt_if_t GATTinterface;

class GATTinstance
{
    protected:
        esp_bt_uuid_t UUID;
        uint16_t Handler;
    public:
        GATTinstance(uint32_t ID)
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

class Characteristic : public GATTinstance
{
    private:
        typedef void (*WriteCallback)(Characteristic*, const uint16_t, const void*);
        typedef void (*ReadCallback)(const Characteristic*, esp_ble_gatts_cb_param_t *param);
    
        esp_attr_value_t  CharData;
        const esp_gatt_perm_t Permition;
        const esp_gatt_char_prop_t Property;

        bool DataAllocatedInsideObject = false;
        void* Data = nullptr;
        size_t DataSize = 0;
        
        ReadCallback ReadHandler = [](const Characteristic* ch, esp_ble_gatts_cb_param_t *param){
            ch->Responce(ch->getData(), ch->getDataSize(), param);
        };
        WriteCallback WriteHandler = [](Characteristic* ch, const uint16_t len, const void* value){
            ch->setData(value, len);
        };
        static uint16_t MTU;
    public:
        Characteristic(uint32_t _UUID, esp_gatt_perm_t, esp_gatt_char_prop_t);
        Characteristic(uint8_t _UUID[ESP_UUID_LEN_128], esp_gatt_perm_t, esp_gatt_char_prop_t);
        
        void setReadCallback(ReadCallback call) { ReadHandler = call; }
        void setWriteCallback(WriteCallback call) { WriteHandler = call; }
        void callReadCallback(esp_ble_gatts_cb_param_t *param);
        void callWriteCallback(esp_ble_gatts_cb_param_t *param);

        void setData(const void* Data, size_t DataSize);
        void setDynamicData(void* Data, size_t DataSize);
        void* getData() const { return Data; }
        size_t getDataSize() const { return DataSize; }

        void Notify(const void* Data, size_t DataSize, uint16_t connected_device_id = 0) const;
        void Responce(const void* Data, size_t DataSize, esp_ble_gatts_cb_param_t* Param) const;

        void AttachToService(uint16_t ServiceHandler);
        static void setMTU(uint16_t);

        ~Characteristic();
};

class Service : public GATTinstance
{
    private:
        esp_gatt_srvc_id_t serviceID;
        
        const std::vector<Characteristic*> Characteristics;
        const size_t CharacteristicsSize;
    public:
        const std::vector<Characteristic*> getCharacteristics() const { return Characteristics; }
        size_t getCharacteristicsSize() const { return CharacteristicsSize; }

        Service(const uint32_t UUID, const std::vector<Characteristic*> Characteristics);
        Service(uint8_t UUID[ESP_UUID_LEN_128], const std::vector<Characteristic*> Characteristics);

        void Start();
        void Create();
};

typedef void GATTScallbackType(esp_ble_gatts_cb_param_t*);
typedef void GAPcallbackType(esp_ble_gap_cb_param_t*);
class ServerDevice
{
    private:
        static const uint8_t MaxEventNumber = 24;

        static const char* Name;
        static esp_ble_adv_params_t AdvertisingParameters;
        static esp_ble_adv_data_t AdvertisingData;
        static esp_ble_adv_data_t ScanResponceData;

        static std::vector<Service*> Services;

        static GATTScallbackType* DeviceCallbacks[MaxEventNumber];
        static GAPcallbackType* GAPcalls[ESP_GAP_BLE_EVT_MAX];
        static void HandleGATTSevents(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
        static void HandleGAPevents(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

        static void Start();

        static uint8_t serviceCounter;
        static uint8_t charCounter;
    public:
        ServerDevice();
        ServerDevice(const char* Name, std::initializer_list<Service*> Services);

        void setGATTSevent(esp_gatts_cb_event_t Event, GATTScallbackType* Callback);
        void setGAPevent(esp_gap_ble_cb_event_t Event, GAPcallbackType* call);

        static void Enable();
        static void Disable();
};