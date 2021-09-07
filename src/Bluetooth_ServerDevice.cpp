#include <Bluetooth.hpp>

ServerDevice::ServerDevice(const char* Name)
{
    this->Name = Name;
}

ServerDevice::ServerDevice()
{
    this->Name = "Unnamed_BLE_Device";
}

 ServerDevice::ServerDevice(const char* Name, esp_ble_adv_data_t adv_data, esp_ble_adv_data_t scan_rsp_data, esp_ble_adv_params_t adv_params, std::vector<Service*> SERVICE)
 {
    this->Name = Name;
    this->adv_data = adv_data;
    this->scan_rsp_data = scan_rsp_data;
    this->adv_params = adv_params;

    this->SERVICE = SERVICE;
 }

void ServerDevice::Start(esp_gatt_if_t GATT_Interface)
{
    esp_ble_gap_set_device_name(Name);
    esp_ble_gap_config_adv_data(&adv_data);
    esp_ble_gap_config_adv_data(&scan_rsp_data);

    for(Service* srv : SERVICE)
    {
        srv->setGATTinterface(GATT_Interface);
        srv->Create();
    }
}


static bool isHandled;
void ServerDevice::DeviceCallback(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) 
    {
    case ESP_GATTS_REG_EVT:
        printf("Services: %i\n", SERVICE.size());
        break;
    case ESP_GATTS_READ_EVT:
    {
        isHandled = false;
        for(Service* serv : SERVICE)
        {
            for(Characteristic* ch : serv->getCharacteristics())
            {
                if(ch->getHandler() == param->read.handle)
                {
                    ch->callReadHandler(param);
                    isHandled = true;
                    break;
                }
            }
            if(isHandled)
                break;
        }
    }   
        break;
    case ESP_GATTS_WRITE_EVT:
    {
        isHandled = false;
        for(Service* serv : SERVICE)
        {
            for(Characteristic* ch : serv->getCharacteristics())
            {
                if(ch->getHandler() == param->write.handle)
                {

                    esp_ble_gatts_send_response(ch->getGATTinterface(), param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);

                    isHandled = true;
                    break;
                }
            }
        }
        if(isHandled)
            break;
    }
        break;
    
    case ESP_GATTS_EXEC_WRITE_EVT:
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        printf("Diconnected. Reason: ");
        switch(param->disconnect.reason)
        {
        case ESP_GATT_CONN_UNKNOWN:                        
            printf("Gatt connection unknown");
            break;
        case ESP_GATT_CONN_L2C_FAILURE:                 
            printf("General L2cap failure");
            break;
        case ESP_GATT_CONN_TIMEOUT:
            printf("Connection timeout ");
            break;
        case ESP_GATT_CONN_TERMINATE_PEER_USER:
            printf("Connection terminate by peer user ");
            break;
        case ESP_GATT_CONN_TERMINATE_LOCAL_HOST:    
            printf("Connection terminated by local host");
            break;
        case ESP_GATT_CONN_FAIL_ESTABLISH:         
            printf("Connection fail to establish " );
            break;
        case ESP_GATT_CONN_LMP_TIMEOUT:            
            printf("Connection fail for LMP response tout");
            break;
        case ESP_GATT_CONN_CONN_CANCEL:            
            printf("L2CAP connection cancelled ");
            break;
        case ESP_GATT_CONN_NONE:  
            printf("None Connection to Cancel");
            break;
        }
        printf("\n");
        esp_ble_gap_start_advertising(&this->adv_params);
        break;

    case ESP_GATTS_CONNECT_EVT: 
    {   
        esp_ble_conn_update_params_t conn_params;
        for(uint8_t i = 0; i < ESP_BD_ADDR_LEN; i++)
            conn_params.bda[i] = param->connect.remote_bda[i];

        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
#ifndef BLE_IOS_PARAMETERS
        conn_params.latency = 0;
        conn_params.max_int = 1000;    // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 6;    // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 600;    // timeout = 400*10ms = 4000ms
#endif
        
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }

    case ESP_GATTS_CREATE_EVT:
        SERVICE[ServiceInitializeCounter]->setHandler(param->create.service_handle);
        SERVICE[ServiceInitializeCounter]->Start();
        printf("Service created - %d with %d chars\n", param->create.service_handle, SERVICE[ServiceInitializeCounter]->getCharacteristics().size());
        AmountOfChars.push_back(SERVICE[ServiceInitializeCounter]->getCharacteristics().size());
        ServiceInitializeCounter++;
        if(ServiceInitializeCounter == SERVICE.size())
        {
            ServiceInitializeCounter = 0;
            CharacteristicInitializeCount = 0;
        }
        //printf("%d\'s chars amount - %d\n", param->create.service_handle, CharacteristicInitializeCountMax);
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
        CharacteristicInitializeCountMax = AmountOfChars[ServiceInitializeCounter];
        printf("\tChar created - %d out of %d with host %d\n", param->add_char.attr_handle, CharacteristicInitializeCountMax, SERVICE[ServiceInitializeCounter]->getHandler());
        SERVICE[ServiceInitializeCounter]->getCharacteristics()[CharacteristicInitializeCount]->setHandler(param->add_char.attr_handle);
        
        CharacteristicInitializeCount++;
        if(CharacteristicInitializeCount == CharacteristicInitializeCountMax)
        {
            CharacteristicInitializeCount = 0;
            ServiceInitializeCounter++;
        }

        break;
    case ESP_GATTS_MTU_EVT:
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