#include <Bluetooth.Server.hpp>
using namespace Bluetooth;

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
    .min_interval = 0x0006, 
    .max_interval = 0x0010, 
    .appearance = 0x00,
    .manufacturer_len = 0, 
    .p_manufacturer_data =  NULL,
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
    .min_interval = 0x0006,
    .max_interval = 0x0010,
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
std::map <uint16_t, Characteristic&> Server::Chars;

GATTScallback* Server::GATTSUserCallbacks[MaxEventNumber]{nullptr};
GATTScallback* Server::MainEvents[MainEventAmount]{nullptr};
GAPcallback*   Server::GAPcalls[ESP_GAP_BLE_EVT_MAX]{nullptr};

void Server::HandleGAPevents(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    if(GAPcalls[event])
        GAPcalls[event](param);

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
    
    ServicesInit();
    ConnectionInit();
    RequestsInit();
}

inline void Server::ServicesInit()
{
    static uint8_t i = 0;
    static uint8_t j = 0;
    // Services are iterated first
    MainEvents[ESP_GATTS_CREATE_EVT] = [](esp_ble_gatts_cb_param_t* param) {
        Services[i]->setHandler(param->create.service_handle);
        Services[i]->Start();

        if(++i == Services.size())
            i = 0;
    };

    // Then all of characteristics in a row
    MainEvents[ESP_GATTS_ADD_CHAR_EVT] = [](esp_ble_gatts_cb_param_t* param) {
        Characteristic& characteristic = Services[i]->getCharacteristic(j);
        
        characteristic.setHandler(param->add_char.attr_handle);
        Chars.insert({param->add_char.attr_handle, characteristic});

        if(++j == Services[i]->getCharsAmount())
        {
            j = 0;
            if(++i == Services.size())
                i = 0;
        }
    };
}

inline void Server::ConnectionInit()
{
    MainEvents[ESP_GATTS_CONNECT_EVT] = [](esp_ble_gatts_cb_param_t* param) {
        static esp_ble_conn_update_params_t fastestParameters{ {0}, 0x06, 0x10, 0, 400 };

        memcpy(fastestParameters.bda, param->connect.remote_bda, ESP_BD_ADDR_LEN);
        esp_ble_gap_update_conn_params(&fastestParameters);
    };

    MainEvents[ESP_GATTS_MTU_EVT] = [](esp_ble_gatts_cb_param_t* param) {
        Characteristic::setMTU(param->mtu.mtu);
    };

    MainEvents[ESP_GATTS_DISCONNECT_EVT] = [](esp_ble_gatts_cb_param_t* param) {
        esp_ble_gap_start_advertising(&AdvertisingParameters);
    };
}

inline void Server::RequestsInit()
{
    MainEvents[ESP_GATTS_READ_EVT] = [](esp_ble_gatts_cb_param_t* param) {
        auto result = Chars.find(param->write.handle);
        if(result != Chars.end())
            result->second.callReadCallback(param);   
        else
            esp_ble_gatts_send_response(GATTinterface, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, NULL);
    };

    MainEvents[ESP_GATTS_WRITE_EVT] = [](esp_ble_gatts_cb_param_t* param) {
        esp_ble_gatts_send_response(GATTinterface, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        auto result = Chars.find(param->write.handle);
        if(result != Chars.end())
            result->second.callWriteCallback(param);
    };

    MainEvents[ESP_GATTS_EXEC_WRITE_EVT] = [](esp_ble_gatts_cb_param_t* param) {
        esp_ble_gatts_send_response(GATTinterface, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
    };
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

    esp_ble_gatts_register_callback(HandleGATTSevents);  
    esp_ble_gap_register_callback(HandleGAPevents);
    esp_ble_gatts_app_register(0);
}

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
}

/**
 * @brief Set device name, GATT interface, advertising data
 */ 
void Server::Start()
{
    esp_ble_gap_set_device_name(Name);
    esp_ble_gap_config_adv_data(&AdvertisingData);
    esp_ble_gap_config_adv_data(&ScanResponceData);

    for(auto service : Services)
        service->Create();
}

void Server::HandleGATTSevents(esp_gatts_cb_event_t event, esp_gatt_if_t gattsif, esp_ble_gatts_cb_param_t *param)
{   

    if(GATTSUserCallbacks[event] != NULL)
        GATTSUserCallbacks[event](param);

    if(event < MainEventAmount && MainEvents[event])
        MainEvents[event](param);

    if(event == ESP_GATTS_REG_EVT)
    {
        GATTinterface = gattsif;
        Start();
    }
}