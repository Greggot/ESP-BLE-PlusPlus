#pragma once
#include <Bluetooth.Characteristic.hpp>
#include <vector>

namespace Bluetooth
{
    class Service : public GATTinstance
    {
    private:
        esp_gatt_srvc_id_t serviceID;
        
        const std::vector<Characteristic*> Characteristics;
        const size_t characteristicAmount;
    public:
        Characteristic& getCharacteristic(size_t index) { return *Characteristics[index]; }
        size_t getCharsAmount() const { return characteristicAmount; }

        Service(const uint32_t UUID, const std::initializer_list<Characteristic*> Characteristics);
        Service(uint8_t UUID[ESP_UUID_LEN_128], const std::initializer_list<Characteristic*> Characteristics);

        void Start();
        void Create();
    };
}