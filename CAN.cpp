#include <build.h>

TaskHandle_t CAN_Receive;

void CAN_send(uint32_t ID, uint8_t* CAN_Data, uint16_t msg_size)
{
    twai_message_t message;
    message.identifier = ID;
    if(ID > 0x7FF)
        message.flags = TWAI_MSG_FLAG_EXTD;
    if(msg_size > 8)
        message.flags |= TWAI_MSG_FLAG_DLC_NON_COMP;
    message.data_length_code = msg_size;
    for(uint16_t i = 0; i < msg_size; i++)
        message.data[i] = CAN_Data[i];

    switch(twai_transmit(&message, pdMS_TO_TICKS(100)))
    {
    case ESP_OK:
        printf("Transmission successfully queued/initiated\n");
        break;
    case ESP_ERR_INVALID_ARG:
        printf("Arguments are invalid\n");
        break;
    case ESP_ERR_TIMEOUT: 
        printf("Timed out waiting for space on TX queue\n");
        break;
    case ESP_FAIL: 
        printf("TX queue is disabled and another message is currently transmitting\n");
        break;
    case ESP_ERR_INVALID_STATE: 
        printf("CAN driver is not in running state, or is not installed\n");
        break;
    case ESP_ERR_NOT_SUPPORTED: 
        printf("Listen Only Mode does not support transmissions\n");
        break;
    }

}

//Queue message for transmission

///     Write some kind of callback function with message as a parameter    ///

void CANreceive_Thread(void * pvParameter)
{
    //uint8_t length;
    //uint8_t i;
    twai_message_t message;
    while(1)
    {
        if (twai_receive(&message, pdMS_TO_TICKS(20)) == ESP_OK) 
        {
            CAN_RX_Callback(message);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

}

bool CAN_Enabled = false;

/** 
 * @brief CAN controller initialization
 * 
 * @param baudrate Speed in KBit/s, create twai_timing_config_t object and initialize it with TWAI_TIMING_CONFIG_<X>KBITS(); macro
 */
void CAN_Init(twai_timing_config_t baudrate)
{
    twai_general_config_t generalConfig = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, TWAI_MODE_NORMAL);
    twai_timing_config_t timeConfig = baudrate;
    twai_filter_config_t filterConfig = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if(twai_driver_install(&generalConfig, &timeConfig, &filterConfig) == ESP_OK)
        printf("CAN has been successfully installed!\n");
    else
        printf("CAN ERROR\n");

    if(twai_start() == ESP_OK)
        printf("CAN driver begins its work...\n");
    else
        printf("Error at a day begining!\n");

    xTaskCreate(CANreceive_Thread, "CANreceive_Thread", 4096, NULL, 0, &CAN_Receive);
    CAN_Enabled = true;
}
 
void CAN_Pause()
{
    vTaskSuspend(CAN_Receive);
}

void CAN_Play()
{
    vTaskResume(CAN_Receive);
}

bool isCANenabled()
{
    return CAN_Enabled;
}