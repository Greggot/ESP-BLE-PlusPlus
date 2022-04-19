#include <Bluetooth.hpp>

#define MAX_MTU 512

Characteristic::Characteristic(uint32_t ID, esp_gatt_perm_t Permition, esp_gatt_char_prop_t Property)
    : GATTinstance(ID), Permition(Permition), Property(Property)
{
}
Characteristic::Characteristic(uint8_t ID[ESP_UUID_LEN_128], esp_gatt_perm_t Permition, esp_gatt_char_prop_t Property)
    : GATTinstance(ID), Permition(Permition), Property(Property)
{
}

/**
 * @brief Create association with Service
*/
esp_err_t Characteristic::AttachToService(uint16_t ServiceHandler)
{
    esp_err_t ret;
    ret = esp_ble_gatts_add_char(ServiceHandler, &UUID, Permition, Property, &Char_Data, NULL);
    
    //  Add descriptor for notifications
    if(Property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
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
    rsp.attr_value.len = DataSize;
    memcpy(rsp.attr_value.value, Data, DataSize);

    esp_ble_gatts_send_response(GATTinterface, Param->read.conn_id, Param->read.trans_id, ESP_GATT_OK, &rsp);
}

/**
 * @brief Notify client with data array
 * @param Data Byte array
 * @param DataSize Number of elements in the array
 * @param ConnectedDeviceID ID of connection
 */ 
void Characteristic::Notify(const void* Data, size_t DataSize, uint16_t ConnectedDeviceID) const
{
    if(Property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
        esp_ble_gatts_send_indicate(GATTinterface, ConnectedDeviceID, Handler, DataSize, (byte*)Data, false);
}

/**
 * @brief Set Inner Characteristic Data by copying. Overwrites old data
 * @param Data Array that will be copied into object's field
 * @param DataSize Size, maximum value - 516
 */
void Characteristic::setData(const void* Data, size_t DataSize)
{
    if(this->Data && DataAllocatedInsideObject)
    {
        byte* DataToClear = (byte*)this->Data;
        delete[] DataToClear;
    }
    DataSize = DataSize > (ESP_GATT_MAX_MTU_SIZE - 1) ? ESP_GATT_MAX_MTU_SIZE - 1 : DataSize;
    this->Data = new byte[DataSize];
    this->DataSize = DataSize;
    
    memcpy(this->Data, Data, this->DataSize);
    DataAllocatedInsideObject = true;
}
/**
 * @brief Set Extern Characteristic Data by reference
 * @param Data Dyncamic data allocated outside of class, DO NOT delete if characterisitc will refer it
 * @param DataSize Size, has no maximum value
*/
void Characteristic::setDynamicData(void* Data, size_t DataSize)
{
    if(this->Data && DataAllocatedInsideObject)
    {
        byte* DataToClear = (byte*)this->Data;
        delete[] DataToClear;
    }
    this->Data = Data;
    this->DataSize = DataSize;
    DataAllocatedInsideObject = false;
}
