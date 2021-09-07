#include <Bluetooth.hpp>

Characteristic::Characteristic()
{ 

    UUID_Init(0x00FF);
    setData(nullptr, 0);

    this->Permition = ESP_GATT_PERM_READ;
    this->Property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
}

Characteristic::Characteristic(uint32_t _UUID)
{ 
    UUID_Init(_UUID);
    setData(nullptr, 0);

    this->Permition = ESP_GATT_PERM_READ;
    this->Property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
}

Characteristic::Characteristic(uint8_t _UUID[ESP_UUID_LEN_128])
{
    for(uint8_t i = 0; i < ESP_UUID_LEN_128; i++) 
        this->UUID.uuid.uuid128[i] = _UUID[i];
    
    this->UUID.len = ESP_UUID_LEN_128;

    setData(nullptr, 0);
    setDataMexLength(600);

    this->Permition = ESP_GATT_PERM_READ;
    this->Property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
}

Characteristic::Characteristic(uint32_t _UUID, esp_gatt_perm_t Permition, esp_gatt_char_prop_t Property)
{
    UUID_Init(_UUID);
    this->Permition = Permition;
    this->Property = Property;

}
Characteristic::Characteristic(uint8_t _UUID[ESP_UUID_LEN_128], esp_gatt_perm_t Permition, esp_gatt_char_prop_t Property)
{
    for(uint8_t i = 0; i < ESP_UUID_LEN_128; i++) 
        this->UUID.uuid.uuid128[i] = _UUID[i];
    
    this->UUID.len = ESP_UUID_LEN_128;

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
            DESCR_DATA_UUID.uuid.uuid16 = 0x2902; 

        ret |= esp_ble_gatts_add_char_descr(ServiceHandler, &DESCR_DATA_UUID, 
                                    ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                    NULL,
                                    NULL);
    }

    //printf("Error: %02X\n", ret);
    return ret;
}

void Characteristic::InfoOut()
{
    printf("UUID: ");
    if(this->UUID.len == ESP_UUID_LEN_16)
        printf("%04X, ", this->UUID.uuid.uuid16);
    else if(this->UUID.len == ESP_UUID_LEN_32)
        printf("%08X, ", this->UUID.uuid.uuid32);

    printf("Handler: %d\n", this->Handler);
}

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

void Characteristic::DefaultReadCallback(Characteristic* ch, esp_ble_gatts_cb_param_t *param)
{
    esp_gatt_rsp_t rsp;
    rsp.handle = ch->getHandler();
    rsp.attr_value.len = ch->getData().attr_len;

    for(int i = 0; i < ch->getData().attr_len; i++)
        rsp.attr_value.value[i] = ch->getData().attr_value[i];

    esp_ble_gatts_send_response(ch->getGATTinterface(), param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
}

void Characteristic::DefaultWriteCallback(Characteristic* ch, esp_ble_gatts_cb_param_t *param)
{
    printf("Write Event Data:");
    for(uint8_t i = 0; i < param->write.len; i++)
        printf("%02X ", param->write.value[i]);
    printf("\n");

    esp_ble_gatts_send_response(ch->getGATTinterface(), param->read.conn_id, param->read.trans_id, ESP_GATT_OK, NULL);
}

/**@brief Notify client with data array
 * 
 * @param Data Byte array
 * @param Data_Length Number of elements in the array
 * @param connected_device_id ID of connection
 */
void Characteristic::Notify(byte* Data, size_t Data_Length, uint16_t connected_device_id)
{
    if((this->Property & ESP_GATT_CHAR_PROP_BIT_NOTIFY) == ESP_GATT_CHAR_PROP_BIT_NOTIFY)
        esp_ble_gatts_send_indicate(GATT_Interface, connected_device_id, Handler, Data_Length, Data, false);
}

void Characteristic::setHandler(uint16_t value)
{
    Handler = value;
}
void Characteristic::setGATTinterface(esp_gatt_if_t value)
{
    GATT_Interface = value;
}

void Characteristic::setDataMexLength(size_t Data_Max_Length)
{
    this->Char_Data.attr_max_len = Data_Max_Length;
}

void Characteristic::setData(byte* Data, size_t Data_Length)
{
    if(Data_Length > this->Char_Data.attr_max_len)
        this->Char_Data.attr_max_len = Data_Length;
    this->Char_Data.attr_len = Data_Length;
    this->Char_Data.attr_value = Data;
}