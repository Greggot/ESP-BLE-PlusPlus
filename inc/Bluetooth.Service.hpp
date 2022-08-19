#pragma once
#include <Bluetooth.Characteristic.hpp>
#include <vector>

class Service : public GATTinstance
{
    private:
        esp_gatt_srvc_id_t serviceID;
        
        const std::vector<Characteristic*> Characteristics;
        const size_t CharacteristicsSize;
    public:
        const std::vector<Characteristic*> getCharacteristics() const { return Characteristics; }
        size_t getCharacteristicsSize() const { return CharacteristicsSize; }

        Service(const uint32_t UUID, const std::vector<Characteristic*> Characteristics);
        Service(uint8_t UUID[ESP_UUID_LEN_128], const std::vector<Characteristic*> Characteristics);

        void Start();
        void Create();
};