#pragma once
#include <Bluetooth.Service.hpp>

#include <esp_gap_ble_api.h>
#include <esp_event.h>

#include <vector>
#include <map>

namespace Bluetooth
{
    typedef void GATTScallbackType(esp_ble_gatts_cb_param_t*);
    typedef void GAPcallbackType(esp_ble_gap_cb_param_t*);
    class ServerDevice
    {
    private:
        static const uint8_t MaxEventNumber = 24;

        static const char* Name;
        static esp_ble_adv_params_t AdvertisingParameters;
        static esp_ble_adv_data_t AdvertisingData;
        static esp_ble_adv_data_t ScanResponceData;

        static std::vector<Service*> Services;

        static GATTScallbackType* DeviceCallbacks[MaxEventNumber];
        static GAPcallbackType* GAPcalls[ESP_GAP_BLE_EVT_MAX];
        static void HandleGATTSevents(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
        static void HandleGAPevents(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

        static void Start();

        static uint8_t serviceCounter;
        static uint8_t charCounter;
        static bool isEnabled;
    public:
        ServerDevice();
        ServerDevice(const char* Name, std::initializer_list<Service*> Services);

        void setGATTSevent(esp_gatts_cb_event_t Event, GATTScallbackType* Callback);
        void setGAPevent(esp_gap_ble_cb_event_t Event, GAPcallbackType* call);

        static bool Status() { return isEnabled; }
        static void Enable();
        static void Disable();
    };
}