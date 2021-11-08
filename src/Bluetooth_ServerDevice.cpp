#include <Bluetooth.hpp>
#include <BluetoothGAP.h>
static const uint8_t MaxEventNumber = 24;

ServerDevice::ServerDevice()
{
    this->Name = "Unnamed_BLE_Device"; 
}

ServerDevice::ServerDevice(const char* Name)
{
    this->Name = Name;
}

ServerDevice::ServerDevice(const char* Name, esp_ble_adv_data_t AdvertisingData, esp_ble_adv_data_t ScanResponceData,
                            esp_ble_adv_params_t AdvertisingParameters, std::vector<Service*> SERVICE)
{
    this->Name = Name;
    this->AdvertisingData = AdvertisingData;
    this->ScanResponceData = ScanResponceData;
    this->AdvertisingParameters = AdvertisingParameters;

    this->SERVICE = SERVICE;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if(esp_bt_controller_init(&bt_cfg) == ESP_OK)
        printf("Controller Initialized\n");

    if(esp_bt_controller_enable(ESP_BT_MODE_BLE) == ESP_OK)
        printf("Controller Enabled\n");
    
    if(esp_bluedroid_init() == ESP_OK)
        printf("Bluedroid Initialized\n");
    
    if(esp_bluedroid_enable() == ESP_OK)
        printf("Bluedroid Enabled\n");
    //esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
}

/**@brief Apply device name, apply GATT interface and set Advertising
 */ 
void ServerDevice::Start(esp_gatt_if_t GATT_Interface)
{
    this->GATT_Interface = GATT_Interface;

    esp_ble_gap_set_device_name(Name);
    esp_ble_gap_config_adv_data(&AdvertisingData);
    esp_ble_gap_config_adv_data(&ScanResponceData);

    for(Service* srv : SERVICE)
    {
        srv->setGATTinterface(GATT_Interface);
        srv->Create();
    }
}

void ServerDevice::DeviceCallback(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    for(int callind : callbackIndexes)
        if(static_cast<int>(event) == callind)
            GATTeventsCallbacks[callind](this, param);

    switch (event) 
    {
    case ESP_GATTS_REG_EVT:
        this->Start(gatts_if);
        break;
    case ESP_GATTS_READ_EVT:
        AllCharacteristics[param->read.handle]->callReadHandler(param);   
        break;
    case ESP_GATTS_WRITE_EVT:
    { 
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        if(AllCharacteristics.find(param->write.handle) != AllCharacteristics.end())
        {
            AllCharacteristics[param->write.handle]->callWriteHandler(param);
        }

        #ifdef BLE_INPUT_PRINTF
        printf("%d Accepted a string (conn id = %d, trans id %d) of %d bytes: ", param->write.handle, param->write.conn_id, param->write.trans_id, param->write.len);
        for(uint16_t i = 0; i < param->write.len; i++)
            printf("%02X ", param->write.value[i]);
        printf("\n");
        #endif
    }
        break;
    case ESP_GATTS_EXEC_WRITE_EVT:
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        printf("Disconnected!\n");
        esp_ble_gap_start_advertising(&adv_params);
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
        SERVICE[ServiceInitializeCounter]->setHandler(param->create.service_handle);
        SERVICE[ServiceInitializeCounter]->Start();

        AllServices.insert({param->create.service_handle, SERVICE[ServiceInitializeCounter]});

        #ifdef BLE_SERVICES_PRINTF
        printf("Service created - %d with %d chars\n", param->create.service_handle, SERVICE[ServiceInitializeCounter]->getCharacteristicsSize());
        #endif

        AmountOfChars.push_back(SERVICE[ServiceInitializeCounter]->getCharacteristicsSize());
        ServiceInitializeCounter++;
        if(ServiceInitializeCounter == SERVICE.size())
        {
            ServiceInitializeCounter = 0;
            CharacteristicInitializeCount = 0;
        }
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
        CharacteristicInitializeCountMax = AmountOfChars[ServiceInitializeCounter];
        #ifdef BLE_CHARACTERISTICS_PRINTF
        printf("\tChar created - %d out of %d with host %d\n", param->add_char.attr_handle, CharacteristicInitializeCountMax, SERVICE[ServiceInitializeCounter]->getHandler());
        #endif

        SERVICE[ServiceInitializeCounter]->getCharacteristics()[CharacteristicInitializeCount]->setHandler(param->add_char.attr_handle);
        AllCharacteristics.insert({param->add_char.attr_handle, SERVICE[ServiceInitializeCounter]->getCharacteristics()[CharacteristicInitializeCount]});

        CharacteristicInitializeCount++;
        if(CharacteristicInitializeCount == CharacteristicInitializeCountMax)
        {
            CharacteristicInitializeCount = 0;
            ServiceInitializeCounter++;
        }

        break;
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
void ServerDevice::setDeviceGATTeventCallback(esp_gatts_cb_event_t Event, BLEcallbackType* Callback)
{
    eventNumber = static_cast<int>(Event);
    if(eventNumber > MaxEventNumber)
        return;
    
    this->GATTeventsCallbacks[eventNumber] = Callback;
    callbackIndexes.push_back(eventNumber);
}