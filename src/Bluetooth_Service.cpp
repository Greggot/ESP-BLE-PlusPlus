#include <Bluetooth.hpp>

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

Service::Service()
{
    this->service_id.is_primary = true;
    this->service_id.id.inst_id = 0x00;
    UUID_Init(0xFF);
    this->service_id.id.uuid = this->UUID;
}

Service::Service(uint32_t _UUID, std::vector<Characteristic*> Characteristics)
{
    this->service_id.is_primary = true;
    this->service_id.id.inst_id = 0x00;
    UUID_Init(_UUID);
    this->service_id.id.uuid = this->UUID;

    this->Characteristics = Characteristics;
}

void Service::GetInfo()
{
    printf("UUID: ");
    if(this->UUID.len == ESP_UUID_LEN_16)
        printf("%04X, ", this->UUID.uuid.uuid16);
    else if(this->UUID.len == ESP_UUID_LEN_32)
        printf("%08X, ", this->UUID.uuid.uuid32);

    printf("Handler: %d\n", this->Handler);
}

void Service::Create()
{
    this->num_handle = 4 + (Characteristics.capacity() << 1);
    if(esp_ble_gatts_create_service(this->GATTinterface, &service_id, num_handle) == ESP_OK)
        printf("\tService Created!\n");
}

void Service::Start()
{
    esp_ble_gatts_start_service(this->Handler);
    for(Characteristic* ch : Characteristics)
    {
        ch->AttachToService(this->Handler);
        ch->setGATTinterface(this->GATTinterface);
    }
}

void Service::setGATTinterface(esp_gatt_if_t GATT_Interface)
{ 
    this->GATTinterface = GATT_Interface; 
    for(Characteristic* ch : Characteristics)
        ch->setGATTinterface(GATT_Interface);
}