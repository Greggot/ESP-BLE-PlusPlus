#pragma once 
#include <Bluetooth.GATT.hpp>

#include <functional>

#define getCorrectSize(size) size > MTU ? MTU : size

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

        
        template<class type>
        void Notify(const type data, size_t size = sizeof(type), uint16_t connID = 0) const {
            if(Property & Prop::Notify && GATTinterface)
                esp_ble_gatts_send_indicate(GATTinterface, connID, Handler, getCorrectSize(size), (uint8_t*)&data, false);
        }
        void Notify(const void* Data, size_t DataSize, uint16_t connected_device_id = 0) const;
        void Responce(const void* Data, size_t DataSize, esp_ble_gatts_cb_param_t* Param) const;

        void AttachToService(uint16_t ServiceHandler);
        static void setMTU(uint16_t);

        ~Characteristic();
};