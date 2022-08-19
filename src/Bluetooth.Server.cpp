#include <Bluetooth.Server.hpp>
using namespace Bluetooth;

Server::Server() {}
bool Server::isEnabled = false;

esp_gatt_if_t BLE::GATTinterface = 0;
const char* Server::Name = "None";
static uint8_t adv_service_uuid128[] = 
{
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 
    0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

esp_ble_adv_data_t Server::AdvertisingData = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

esp_ble_adv_data_t Server::ScanResponceData = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    //.min_interval = 0x0006,
    //.max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0, 
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

esp_ble_adv_params_t Server::AdvertisingParameters = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

std::vector<Service*> Server::Services;
GATTScallbackType* Server::DeviceCallbacks[MaxEventNumber] = {NULL};
GAPcallbackType*   Server::GAPcalls[ESP_GAP_BLE_EVT_MAX] = {nullptr};

void Server::HandleGAPevents(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    printf("GAP event %u\n", (int)event);
    if(GAPcalls[(int)event])
        GAPcalls[(int)event](param);

    switch (event) 
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            if(isEnabled)
            esp_ble_gap_start_advertising(&AdvertisingParameters);
        break;
    default:
        break;
    }
}

Server::Server(const char* Name, std::initializer_list<Service*> ServiceList)
{
    this->Name = Name;
    for(auto service : ServiceList)
        Services.push_back(service);
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
}

void Server::Enable()
{
    if(isEnabled)
        return;
    isEnabled = true;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable();

    esp_ble_gatts_register_callback(Server::HandleGATTSevents);  
    esp_ble_gap_register_callback(Server::HandleGAPevents);
    esp_ble_gatts_app_register(0);
}


uint8_t Server::serviceCounter = 0;
uint8_t Server::charCounter = 0;

void Server::Disable()
{
    if(!isEnabled)
        return;
    isEnabled = false;
    printf("Disabling BLE...\n");
    esp_ble_gatts_close(GATTinterface, 0);
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    GATTinterface = 0;
    serviceCounter = 0;
    charCounter = 0;

}

/**
 * @brief Set device name, GATT interface, advertising data
 */ 
void Server::Start()
{
    esp_ble_gap_set_device_name(Name);
    esp_ble_gap_config_adv_data(&AdvertisingData);
    esp_ble_gap_config_adv_data(&ScanResponceData);

    for(auto srv : Services)
        srv->Create();
}

void Server::HandleGATTSevents(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{   
    static std::map <uint16_t, Characteristic*> Chars;
    static std::map <uint16_t, Service*> AllServices;

    if(DeviceCallbacks[(int)event] != NULL)
        DeviceCallbacks[(int)event](param);

    switch (event) 
    {
    case ESP_GATTS_REG_EVT:
        GATTinterface = gatts_if;
        Start();
        break;
    case ESP_GATTS_READ_EVT:
    {
        // Will crash without searching - nullptr can't call anything
        auto result = Chars.find(param->write.handle);
        if(result != Chars.end())
            result->second->callReadCallback(param);   
        else
            esp_ble_gatts_send_response(GATTinterface, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, NULL);
        break;
    }
    case ESP_GATTS_WRITE_EVT:
    { 
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        // Will crash without searching - nullptr can't call anything
        auto result = Chars.find(param->write.handle);
        if(result != Chars.end())
            result->second->callWriteCallback(param);

        #ifdef BLE_INPUT_PRINTF
        if(param->write.len < MAX_OUTPUT_AMOUNT)
        {
            printf("%d Accepted a string (conn id = %d, trans id %d) of %d bytes: ", param->write.handle, param->write.conn_id, param->write.trans_id, param->write.len);
            for(uint16_t i = 0; i < param->write.len; ++i)
                printf("%02X ", param->write.value[i]);
            printf("\n");
        }
        else
            printf("%d <- %dB (%d)\n", param->write.handle, param->write.len, param->write.trans_id);
        #endif
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        printf("Disconnected!\n");
        esp_ble_gap_start_advertising(&AdvertisingParameters);
        break;

    case ESP_GATTS_CONNECT_EVT: 
    {   
        // These're the fastest parameters with a speed of 100KB/18s (5.5KB/s)
        static const uint16_t MaxInt  = 0x10;
        static const uint16_t MinInt  = 6;
        static const uint16_t Timeout = 400; 
        static esp_ble_conn_update_params_t ConnectionParameters{ {0}, MinInt, MaxInt, 0, Timeout };
        
        memcpy(ConnectionParameters.bda, param->connect.remote_bda, ESP_BD_ADDR_LEN);
        esp_ble_gap_update_conn_params(&ConnectionParameters);
        break;
    }

    case ESP_GATTS_CREATE_EVT:
    {
        Services[serviceCounter]->setHandler(param->create.service_handle);
        Services[serviceCounter]->Start();

        AllServices.insert({param->create.service_handle, Services[serviceCounter]});

        #ifdef BLE_SERVICES_PRINTF 
        Services[serviceCounter]->ConsoleInfoOut();
        #endif

        if(++serviceCounter == Services.size())
            serviceCounter = 0;
        break;
    }
    case ESP_GATTS_ADD_CHAR_EVT:
    {
        Characteristic* ch = Services[serviceCounter]->getCharacteristics()[charCounter];
        
        ch->setHandler(param->add_char.attr_handle);
        Chars.insert({param->add_char.attr_handle, ch});

        #ifdef BLE_CHARACTERISTICS_PRINTF
        printf("\t");
        ch->ConsoleInfoOut();
        #endif

        if(++charCounter == Services[serviceCounter]->getCharacteristicsSize())
        {
            charCounter = 0;
            ++serviceCounter;
        }
        break;
    }
    case ESP_GATTS_MTU_EVT:
        printf("New MTU size: %d\n", param->mtu.mtu);
        Characteristic::setMTU(param->mtu.mtu);
        break;
    default:
        break;
    }
}

void Server::setGATTSevent(esp_gatts_cb_event_t Event, GATTScallbackType* call)
{
    DeviceCallbacks[(int)Event] = call;
}

void Server::setGAPevent(esp_gap_ble_cb_event_t Event, GAPcallbackType* call)
{
    GAPcalls[(int)Event] = call;
}
