#include <Bluetooth/Service.hpp>
using namespace Bluetooth;

Service::Service(const uint32_t ID, const std::initializer_list<Characteristic*> Characteristics)
    : GATTinstance(ID), Characteristics(Characteristics), characteristicAmount(Characteristics.size())
{
    serviceID.is_primary = true;
    serviceID.id.inst_id = 0x00;
    serviceID.id.uuid = UUID;
}

Service::Service(uint8_t ID[ESP_UUID_LEN_128], const std::initializer_list<Characteristic*> Characteristics)
    : GATTinstance(ID), Characteristics(Characteristics), characteristicAmount(Characteristics.size())
{
    serviceID.is_primary = true;
    serviceID.id.inst_id = 0x00;
    serviceID.id.uuid = UUID;
}

void Service::Create()
{
    esp_ble_gatts_create_service(GATTinterface, &serviceID, 4 + (characteristicAmount << 1));
}

void Service::Start()
{
    esp_ble_gatts_start_service(Handler);
    for(auto ch : Characteristics)
        ch->AttachToService(Handler);
}