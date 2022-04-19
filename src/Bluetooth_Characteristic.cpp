#include <Bluetooth.hpp>

#define MAX_MTU 517
#define MIN_MTU 23
#define getCorrectSize(size) size > MTU ? MTU : size
uint16_t Characteristic::MTU = MIN_MTU;

Characteristic::Characteristic(uint32_t ID, esp_gatt_perm_t Permition, esp_gatt_char_prop_t Property)
    : GATTinstance(ID), Permition(Permition), Property(Property)
{
}
Characteristic::Characteristic(uint8_t ID[ESP_UUID_LEN_128], esp_gatt_perm_t Permition, esp_gatt_char_prop_t Property)
    : GATTinstance(ID), Permition(Permition), Property(Property)
{
}

static inline void AddNotificationDescriptor(uint16_t ServiceHandler)
{
    
    static esp_bt_uuid_t DescriptorUUID{
        .len = ESP_UUID_LEN_16,
        { .uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG, }
    };

    esp_ble_gatts_add_char_descr(ServiceHandler, &DescriptorUUID,  
        Perm::Read | Perm::Write, NULL, NULL);
}

/**
 * @brief Create association with Service
*/
void Characteristic::AttachToService(uint16_t ServiceHandler)
{
    esp_ble_gatts_add_char(ServiceHandler, &UUID, Permition, Property, &CharData, NULL);
    
    if(Property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
        AddNotificationDescriptor(ServiceHandler);
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
void Characteristic::Responce(const void* Data, size_t DataSize, esp_ble_gatts_cb_param_t* Param) const
{
    static esp_gatt_rsp_t rsp;
    
    rsp.handle = Handler;
    rsp.attr_value.len = getCorrectSize(DataSize);
    memcpy(rsp.attr_value.value, Data, rsp.attr_value.len);

    esp_ble_gatts_send_response(GATTinterface, Param->read.conn_id, Param->read.trans_id, ESP_GATT_OK, &rsp);
}

/**
 * @brief Notify without confirmation
 */ 
void Characteristic::Notify(const void* Data, size_t DataSize, uint16_t ConnectedDeviceID) const
{
    if(Property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
        esp_ble_gatts_send_indicate(GATTinterface, ConnectedDeviceID, Handler, getCorrectSize(DataSize), (uint8_t*)Data, false);
}

/**
 * @brief Allocate array inside object and copy data to it
 */
void Characteristic::setData(const void* Data, size_t DataSize)
{
    if(this->Data && DataAllocatedInsideObject)
        delete[] (uint8_t*)this->Data;
    this->DataSize = getCorrectSize(DataSize);

    this->Data = new byte[DataSize];    
    memcpy(this->Data, Data, this->DataSize);
    DataAllocatedInsideObject = true;
}
/**
 * @brief Set reference to data allocated by caller. DO NOT delete data while 
 * it is being referenced, if you don't look for a trouble
*/
void Characteristic::setDynamicData(void* Data, size_t DataSize)
{
    if(this->Data && DataAllocatedInsideObject)
        delete[] (uint8_t*)this->Data;
    this->DataSize = getCorrectSize(DataSize);

    this->Data = Data;
    DataAllocatedInsideObject = false;
}

void Characteristic::setMTU(uint16_t MTU)
{
    if(MTU <= MAX_MTU && MTU >= MIN_MTU)
        Characteristic::MTU = MTU;
}

Characteristic::~Characteristic()
{
    if(this->Data && DataAllocatedInsideObject)
        delete[] (uint8_t*)this->Data;
}
