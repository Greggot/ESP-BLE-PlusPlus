#include <Bluetooth.hpp>

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

void Service::Create()
{
    this->num_handle = 4 + (CharacteristicsSize << 1);
    esp_ble_gatts_create_service(GATTinterface, &service_id, num_handle);
}

void Service::Start()
{
    esp_ble_gatts_start_service(Handler);
    for(size_t i = 0; i < CharacteristicsSize; i++)
        Characteristics[i]->AttachToService(Handler);
}