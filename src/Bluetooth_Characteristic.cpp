#include <Bluetooth.hpp>
#include <cstring>

#define MAX_MTU 22

Characteristic::Characteristic()
{ 

    UUID_Init(0x00FF);
    
    this->Data = nullptr;
    this->DataSize = 0;

    this->Permition = ESP_GATT_PERM_READ;
    this->Property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
}

Characteristic::Characteristic(uint32_t _UUID)
{ 
    UUID_Init(_UUID);
    this->Data = nullptr;
    this->DataSize = 0;

    this->Permition = ESP_GATT_PERM_READ;
    this->Property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
}

Characteristic::Characteristic(uint8_t _UUID[ESP_UUID_LEN_128])
{
    for(uint8_t i = 0; i < ESP_UUID_LEN_128; i++) 
        this->UUID.uuid.uuid128[i] = _UUID[i];
    
    this->UUID.len = ESP_UUID_LEN_128;

    this->Data = nullptr;
    this->DataSize = 0;

    this->Permition = ESP_GATT_PERM_READ;
    this->Property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
}

Characteristic::Characteristic(uint32_t _UUID, esp_gatt_perm_t Permition, esp_gatt_char_prop_t Property)
{
    UUID_Init(_UUID);
    this->Data = nullptr;
    this->DataSize = 0;
    this->Permition = Permition;
    this->Property = Property;

}
Characteristic::Characteristic(uint8_t _UUID[ESP_UUID_LEN_128], esp_gatt_perm_t Permition, esp_gatt_char_prop_t Property)
{
    for(uint8_t i = 0; i < ESP_UUID_LEN_128; i++) 
        this->UUID.uuid.uuid128[i] = _UUID[i];
    
    this->UUID.len = ESP_UUID_LEN_128;
    this->Data = nullptr;
    this->DataSize = 0;

    this->Permition = Permition;
    this->Property = Property;
}

esp_err_t Characteristic::AttachToService(uint16_t ServiceHandler)
{
    esp_err_t ret;
    ret = esp_ble_gatts_add_char(ServiceHandler, &UUID, Permition, Property, &Char_Data, NULL);
    if((this->Property & ESP_GATT_CHAR_PROP_BIT_NOTIFY) == ESP_GATT_CHAR_PROP_BIT_NOTIFY)
    {
        esp_bt_uuid_t DESCR_DATA_UUID;
            DESCR_DATA_UUID.len = ESP_UUID_LEN_16;
            DESCR_DATA_UUID.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG; 

        ret |= esp_ble_gatts_add_char_descr(ServiceHandler, &DESCR_DATA_UUID, 
                                    ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                    NULL,
                                    NULL);
    }
    return ret;
}

#ifdef BLE_CHARACTERISTICS_PRINTF
void Characteristic::ConsoleInfoOut()
{
    printf("UUID: ");
    if(this->UUID.len == ESP_UUID_LEN_16)
        printf("%04X, ", this->UUID.uuid.uuid16);
    else if(this->UUID.len == ESP_UUID_LEN_32)
        printf("%08X, ", this->UUID.uuid.uuid32);

    printf("Handler: %d\n", this->Handler);
}
#endif

void Characteristic::UUID_Init(uint32_t _UUID)
{
    if(_UUID <= 0xFFFF)
    {
        this->UUID.len = ESP_UUID_LEN_16;
        this->UUID.uuid.uuid16 = _UUID;
    }
    else
    {
        this->UUID.len = ESP_UUID_LEN_32;
        this->UUID.uuid.uuid32 = _UUID;
    }
}

void Characteristic::callReadHandler(esp_ble_gatts_cb_param_t *param)
{
    this->ReadHandler(this, param);
}

void Characteristic::callWriteHandler(esp_ble_gatts_cb_param_t *param)
{
    this->WriteHandler(this, param);
}

void Characteristic::DefaultReadCallback(Characteristic* ch, esp_ble_gatts_cb_param_t *param)
{
    esp_gatt_rsp_t rsp;
    rsp.handle = ch->getHandler();
    rsp.attr_value.len = ch->getDataSize();
    memcpy(rsp.attr_value.value, ch->getData(), ch->getDataSize());
    esp_ble_gatts_send_response(ch->getGATTinterface(), param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
}

void Characteristic::DefaultWriteCallback(Characteristic* ch, esp_ble_gatts_cb_param_t *param)
{
    ch->setData(param->write.value, param->write.len);
}

/**@brief Notify client with data array
 * 
 * @param Data Byte array
 * @param Data_Length Number of elements in the array
 * @param connected_device_id ID of connection
 */ 
void Characteristic::Notify(byte* Data, size_t DataSize, uint16_t ConnectedDeviceID)
{
    if(this->Property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
        esp_ble_gatts_send_indicate(GATTinterface, ConnectedDeviceID, Handler, DataSize, Data, false);
}

/**@brief Set Inner Characteristic Data 
 * @param Data Dynamic allocated array
 * @param DataSize Size, maximum value - 516
 * 
 */
void Characteristic::setData(const byte* Data, size_t DataSize)
{
    if(this->Data != nullptr)
    {
        byte* DataToClear = this->Data;
        delete(DataToClear);
    }
    DataSize = DataSize > (ESP_GATT_MAX_MTU_SIZE - 1) ? ESP_GATT_MAX_MTU_SIZE - 1 : DataSize;
    this->Data = new byte[DataSize];
    this->DataSize = DataSize;
    
    memcpy(this->Data, Data, this->DataSize);
}