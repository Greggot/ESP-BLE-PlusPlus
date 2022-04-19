#include <Bluetooth.hpp>

Service::Service(const uint32_t ID, const std::vector<Characteristic*> Characteristics)
    : GATTinstance(ID), Characteristics(Characteristics), CharacteristicsSize(Characteristics.size())
{
    service_id.is_primary = true;
    service_id.id.inst_id = 0x00;
    service_id.id.uuid = UUID;
}

void Service::Create()
{
    esp_ble_gatts_create_service(GATTinterface, &service_id, 4 + (CharacteristicsSize << 1));
}

void Service::Start()
{
    esp_ble_gatts_start_service(Handler);
    for(auto ch : Characteristics)
        ch->AttachToService(Handler);
}