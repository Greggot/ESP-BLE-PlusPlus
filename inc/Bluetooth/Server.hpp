#pragma once
#include "Service.hpp"

#include <esp_gap_ble_api.h>
#include <esp_event.h>

#include <vector>
#include <map>

namespace Bluetooth
{
    typedef void GATTScallback(esp_ble_gatts_cb_param_t*);
    typedef void GAPcallback(esp_ble_gap_cb_param_t*);
    class Server
    {
    private:
        static const uint8_t MaxEventNumber = 24;
        static const uint8_t MainEventAmount = 16;

        static const char* Name;
        static esp_ble_adv_params_t AdvertisingParameters;
        static esp_ble_adv_data_t AdvertisingData;
        static esp_ble_adv_data_t ScanResponceData;

        static std::vector<Service*> Services;
        static std::map <uint16_t, Characteristic&> Chars;

        static GATTScallback* GATTSUserCallbacks[MaxEventNumber];
        static GATTScallback* MainEvents[MainEventAmount];
        static GAPcallback* GAPcalls[ESP_GAP_BLE_EVT_MAX];
        static void HandleGATTSevents(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
        static void HandleGAPevents(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

        static void Start();

        static bool isEnabled;

        static inline void ServicesInit();
        static inline void ConnectionInit();
        static inline void RequestsInit();
    public:
        Server(const char* Name, std::initializer_list<Service*> Services);

        void Set(esp_gatts_cb_event_t event, GATTScallback* call) { GATTSUserCallbacks[event] = call; }
        void Set(esp_gap_ble_cb_event_t event, GAPcallback* call) { GAPcalls[event] = call; }

        static bool Status() { return isEnabled; }
        static void Enable();
        static void Disable();
    };
}