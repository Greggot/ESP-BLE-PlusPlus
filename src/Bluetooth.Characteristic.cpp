#include <Bluetooth.Characteristic.hpp>
using namespace Bluetooth;

#define MAX_MTU 517
#define MIN_MTU 23
uint16_t Characteristic::MTU = MIN_MTU;

Characteristic::Characteristic(uint32_t UUID, esp_gatt_perm_t Permition, esp_gatt_char_prop_t Property)
    : GATTinstance(UUID), Permition(Permition), Property(Property)
{
}
Characteristic::Characteristic(uint8_t UUID[ESP_UUID_LEN_128], esp_gatt_perm_t Permition, esp_gatt_char_prop_t Property)
    : GATTinstance(UUID), Permition(Permition), Property(Property)
{
}

static inline void AddNotificationDescriptor(uint16_t ServiceHandler)
{
    static esp_bt_uuid_t DescriptorUUID{
        .len = ESP_UUID_LEN_16,
        { .uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG, }
    };

    esp_ble_gatts_add_char_descr(ServiceHandler, &DescriptorUUID,  
        permition::Read | permition::Write, NULL, NULL);
}

/**
 * @brief Create association with Service
*/
void Characteristic::AttachToService(uint16_t serviceHandler)
{
    esp_ble_gatts_add_char(serviceHandler, &UUID, Permition, Property, &CharData, NULL);
    
    if(Property & property::Notify)
        AddNotificationDescriptor(serviceHandler);
}

void Characteristic::callReadCallback(esp_ble_gatts_cb_param_t *param)
{
    ReadHandler(this, param);
}

void Characteristic::callWriteCallback(esp_ble_gatts_cb_param_t *param)
{
    WriteHandler(this, param->write.len, param->write.value);
}

/**
 * @brief Answer to read request
*/
void Characteristic::Responce(const void* data, size_t size, esp_ble_gatts_cb_param_t* p) const
{
    static esp_gatt_rsp_t responce;
    
    responce.handle = Handler;
    responce.attr_value.len = cropToMTU(size);
    memcpy(responce.attr_value.value, data, responce.attr_value.len);

    esp_ble_gatts_send_response(GATTinterface, p->read.conn_id, p->read.trans_id, ESP_GATT_OK, &responce);
}

/**
 * @brief Notify without confirmation
 */ 
void Characteristic::Notify(const void* data, size_t size, uint16_t deviceID) const
{
    if(Property & property::Notify && GATTinterface)
        esp_ble_gatts_send_indicate(GATTinterface, deviceID, Handler, cropToMTU(size), (uint8_t*)data, false);
}

/**
 * @brief Allocate array inside object and copy data to it
 */
void Characteristic::setData(const void* data, size_t size)
{
    if(Data && DataAllocatedInsideObject)
        delete[] (uint8_t*)Data;
    DataSize = cropToMTU(size);

    Data = new uint8_t[DataSize];    
    memcpy(Data, data, DataSize);
    DataAllocatedInsideObject = true;
}
/**
 * @brief Set reference to data allocated by caller. DO NOT delete data while 
 * it is being referenced, if you don't look for a trouble
*/
void Characteristic::setDynamicData(void* data, size_t size)
{
    if(Data && DataAllocatedInsideObject)
        delete[] (uint8_t*)Data;
    DataSize = cropToMTU(size);

    Data = data;
    DataAllocatedInsideObject = false;
}

void Characteristic::setMTU(uint16_t MTU)
{
    if(MTU <= MAX_MTU && MTU >= MIN_MTU)
        Characteristic::MTU = MTU;
}

Characteristic::~Characteristic()
{
    if(Data && DataAllocatedInsideObject)
        delete[] (uint8_t*)Data;
}
