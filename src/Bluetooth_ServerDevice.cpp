#include <Bluetooth.hpp>

ServerDevice::ServerDevice() {}

esp_gatt_if_t GATTinterface = 0;
const char* ServerDevice::Name = "None";
static uint8_t adv_service_uuid128[] = 
{
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 
    0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

esp_ble_adv_data_t ServerDevice::AdvertisingData = {
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

esp_ble_adv_data_t ServerDevice::ScanResponceData = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    //.min_interval = 0x0006,
    //.max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

esp_ble_adv_params_t ServerDevice::AdvertisingParameters = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

ServerDevice::ServerDevice(const char* Name, std::initializer_list<Service*> ServiceList)
{
    this->Name = Name;
    for(auto service : ServiceList)
        Services.push_back(service);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable();
}

/**
 * @brief Set device name, GATT interface, advertising data
 */ 
void ServerDevice::Start()
{
    esp_ble_gap_set_device_name(Name);
    esp_ble_gap_config_adv_data(&AdvertisingData);
    esp_ble_gap_config_adv_data(&ScanResponceData);

    for(auto srv : Services)
        srv->Create();
}

void ServerDevice::HandleGATTSevents(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    
    static uint16_t ServiceInitializeCounter = 0;
    static uint16_t CharacteristicInitializeCount = 0;
    static uint16_t CharacteristicInitializeCountMax;
    
    static std::map <uint16_t, Characteristic*> AllCharacteristics;
    static std::map <uint16_t, Service*> AllServices;

    int eventNumber = static_cast<int>(event);
    if(eventNumber < MaxEventNumber && DeviceCallbacks[eventNumber] != NULL)
        DeviceCallbacks[eventNumber](this, param);

    switch (event) 
    {
    case ESP_GATTS_REG_EVT:
        GATTinterface = gatts_if;
        Start();
        break;
    case ESP_GATTS_READ_EVT:
        AllCharacteristics[param->read.handle]->callReadCallback(param);   
        break;
    case ESP_GATTS_WRITE_EVT:
    { 
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        if(AllCharacteristics.find(param->write.handle) != AllCharacteristics.end())
        {
            AllCharacteristics[param->write.handle]->callWriteCallback(param);
        }

        #ifdef BLE_INPUT_PRINTF
        if(param->write.len < 40)
        {
            printf("%d Accepted a string (conn id = %d, trans id %d) of %d bytes: ", param->write.handle, param->write.conn_id, param->write.trans_id, param->write.len);
            for(uint16_t i = 0; i < param->write.len; i++)
                printf("%02X ", param->write.value[i]);
            printf("\n");
        }
        else
            printf("%d <- %dB (%d)\n", param->write.handle, param->write.len, param->write.trans_id);
        #endif
    }
        break;
    case ESP_GATTS_EXEC_WRITE_EVT:
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        printf("Disconnected!\n");
        esp_ble_gap_start_advertising(&AdvertisingParameters);
        break;

    case ESP_GATTS_CONNECT_EVT: 
    {   
        esp_ble_conn_update_params_t conn_params;
        for(uint8_t i = 0; i < ESP_BD_ADDR_LEN; i++)
            conn_params.bda[i] = param->connect.remote_bda[i];

        // These're the fastest parameters with a speed of 100KB/18s (5.5KB/s)
        conn_params.latency = 0;
        conn_params.max_int = 0x10;    // max_int = 0x10*1.25ms = 20ms
        conn_params.min_int = 6;    // min_int = 6*1.25ms = 7.5ms, Android minimum
        conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
        
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }

    case ESP_GATTS_CREATE_EVT:
        Services[ServiceInitializeCounter]->setHandler(param->create.service_handle);
        Services[ServiceInitializeCounter]->Start();

        AllServices.insert({param->create.service_handle, Services[ServiceInitializeCounter]});

        #ifdef BLE_SERVICES_PRINTF 
        Services[ServiceInitializeCounter]->ConsoleInfoOut();
        #endif

        ServiceInitializeCounter++;
        if(ServiceInitializeCounter == Services.size())
        {
            ServiceInitializeCounter = 0;
            CharacteristicInitializeCount = 0;
        }
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
    {
        CharacteristicInitializeCountMax = Services[ServiceInitializeCounter]->getCharacteristicsSize();        
        Characteristic* ch = Services[ServiceInitializeCounter]->getCharacteristics()[CharacteristicInitializeCount];
        ch->setHandler(param->add_char.attr_handle);
        AllCharacteristics.insert({param->add_char.attr_handle, ch});

        #ifdef BLE_CHARACTERISTICS_PRINTF
        printf("\t");
        ch->ConsoleInfoOut();
        #endif

        CharacteristicInitializeCount++;
        if(CharacteristicInitializeCount == CharacteristicInitializeCountMax)
        {
            CharacteristicInitializeCount = 0;
            ServiceInitializeCounter++;
        }

        break;
    }
    case ESP_GATTS_MTU_EVT:
        printf("New MTU size: %d\n", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
    case ESP_GATTS_DELETE_EVT:
    case ESP_GATTS_START_EVT:
    case ESP_GATTS_STOP_EVT:
    case ESP_GATTS_CONF_EVT:
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

static int eventNumber;
void ServerDevice::setGATTSevent(esp_gatts_cb_event_t Event, GATTScallbackType* Callback)
{
    eventNumber = static_cast<int>(Event);
    if(eventNumber > MaxEventNumber)
        return;
    
    this->DeviceCallbacks[eventNumber] = Callback;
}