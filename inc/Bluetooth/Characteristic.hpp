#pragma once 
#include "GATT.hpp"

#include <functional>

#define getCorrectSize(size) size > MTU ? MTU : size

namespace Bluetooth
{

    class Characteristic : public GATTinstance
    {
    private:
        typedef std::function<void(const Characteristic*, esp_ble_gatts_cb_param_t*)> ReadCallback;
        typedef std::function<void(Characteristic*, const uint16_t, const void*)> WriteCallback;
    
        esp_attr_value_t  CharData;
        const esp_gatt_perm_t Permition;
        const esp_gatt_char_prop_t Property;

        bool DataAllocatedInsideObject = false;
        void* Data = nullptr;
        size_t DataSize = 0;
        
        static uint16_t MTU;
        inline uint16_t cropToMTU(uint16_t size) const { return std::min(MTU, size); }

        ReadCallback ReadHandler = [](const Characteristic* ch, esp_ble_gatts_cb_param_t *param){
            ch->Responce(ch->getData(), ch->getDataSize(), param);
        };
        WriteCallback WriteHandler = [](Characteristic* ch, const uint16_t len, const void* value){
            ch->setData(value, len);
        };
    public:
        Characteristic(uint32_t UUID, esp_gatt_perm_t, esp_gatt_char_prop_t);
        Characteristic(uint8_t UUID[ESP_UUID_LEN_128], esp_gatt_perm_t, esp_gatt_char_prop_t);
        
        void setReadCallback(ReadCallback call) { ReadHandler = call; }
        void setWriteCallback(WriteCallback call) { WriteHandler = call; }
        void callReadCallback(esp_ble_gatts_cb_param_t *param);
        void callWriteCallback(esp_ble_gatts_cb_param_t *param);

        void setData(const void* data, size_t size);
        void setDynamicData(void* data, size_t size);
        void* getData() const { return Data; }
        size_t getDataSize() const { return DataSize; }

        template<class type>
        void Notify(const type& data, size_t size = sizeof(type), uint16_t deviceID = 0) const {
            if(Property & property::Notify && GATTinterface)
                esp_ble_gatts_send_indicate(GATTinterface, deviceID, Handler, cropToMTU(size), (uint8_t*)&data, false);
        }
        void Notify(const void* data, size_t size, uint16_t deviceID = 0) const;
        void Responce(const void* data, size_t size, esp_ble_gatts_cb_param_t* p) const;

        void AttachToService(uint16_t serviceHandler);
        static void setMTU(uint16_t);

        ~Characteristic();
    };
}