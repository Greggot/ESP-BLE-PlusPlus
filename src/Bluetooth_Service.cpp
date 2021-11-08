#include <Bluetooth.hpp>
#include <cstring>

void Service::UUID_Init(uint32_t _UUID)
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

Service::Service(uint32_t _UUID, std::vector<Characteristic*> Characteristics)
{
    this->service_id.is_primary = true;
    this->service_id.id.inst_id = 0x00;
    
    this->UUID.len = _UUID & 0xFFFF0000 ? sizeof(uint32_t) : sizeof(uint16_t);
    memcpy(&this->UUID.uuid, &_UUID, this->UUID.len);
    this->service_id.id.uuid = this->UUID;

    this->Characteristics = Characteristics;
    this->CharacteristicsSize = Characteristics.size();
}

#ifdef BLE_SERVICES_PRINTF
void Service::ConsoleInfoOut()
{
    printf("UUID: ");
    if(this->UUID.len == ESP_UUID_LEN_16)
        printf("%04X, ", this->UUID.uuid.uuid16);
    else if(this->UUID.len == ESP_UUID_LEN_32)
        printf("%08X, ", this->UUID.uuid.uuid32);

    printf("Handler: %d\n", this->Handler);
}
#endif

void Service::Create()
{
    this->num_handle = 4 + (CharacteristicsSize << 1);
    esp_ble_gatts_create_service(this->GATTinterface, &service_id, num_handle);
}

void Service::Start()
{
    esp_ble_gatts_start_service(this->Handler);
    for(size_t i = 0; i < CharacteristicsSize; i++)
    {
        Characteristics[i]->AttachToService(this->Handler);
        Characteristics[i]->setGATTinterface(this->GATTinterface);
    }
}

void Service::setGATTinterface(esp_gatt_if_t GATT_Interface)
{ 
    this->GATTinterface = GATT_Interface; 
    for(size_t i = 0; i < CharacteristicsSize; i++)
        Characteristics[i]->setGATTinterface(GATT_Interface);
}