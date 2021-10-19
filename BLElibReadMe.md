# ESP-IDF based BLE GATT Server C++ library

## What does library can do

- Initialize BLE GATT server device with name and a set of services & characteriscs
- Define callbacks for device and characteristic events
- Use notifications to send data to client

## Library structure

```text
ServerDevice - BLE point with name and address
|---Service - Abstract collection of 'values'
|---Service
|---|---Characteristic - 'Value' with callbacks
|---Service
|---|---Characteristic
|---|---Characteristic
```

Original ESP-IDF Bluetooth library is too complicated to work with: almoust no abstraction mechanism is present. Thus, this library make sure to enable `bluedroid` and advertising by itself; let user write independed callback functions with no need to implement one giant `swithc-case` construction.

### Hierarchy

`ServiceDevice` class has an `std::map` for each service and characteristic added to it. It associates a 16-bit `handler` with `Service` and `Characteristic` objects. Each `Service` have a `std::vector` of its characteristics, making it possible to accept all abstractly united 'values'.

### Callbacks

Each `Characteristic` has `ReadHandler` and `WriteHandler`. These are set via `public` setters, which tell object to call sertain function when one of {read, write} events happen.

`ServerDevice` has an array of pointers to functions, which'll be called if they're non empty. It is in this way because class itself is a wrap around static function which is called each time any GATT event occures. This array's elements are set by user and is called via class function. To make it happen user stil needs to do this:

```C++
//Make ServerDevice visible to all main functions
static ServerDevice BLE_Kit;
//This function will take over GATT events
static void ServerDeviceEventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    //All events are handled through this function
    BLE_Kit.HandleEvent(event, gatts_if, param);
}

void app_main()
{
    std::vector<Service*> Services;
    //Constructor
    BLE_Kit = ServerDevice("Name", adv_data, scan_rsp_data, adv_params, Services);

    //Register event function withesp-idf function
    esp_ble_gatts_register_callback(ServerDeviceEventHandler);
}
```

Also theres GAP events, bigger part of which is different kinds of 'Connection lost', so you need to add these to `app_main`

```C++
//Register callback main purpose of which is to start advertising after disconnect
esp_ble_gap_register_callback(gap_event_handler);
//Register single device
esp_ble_gatts_app_register(0);
```

## Characteristic usage

Consider

```C++
static Characteristic TimeChar = Characteristic(0x2A2B, ESP_GATT_PERM_WRITE | ESP_GATT_PERM_READ, ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ);
```

global visible to different functions object. It has an UUID of `0x2A2B` and set of permitions and properties, defined by idf bluetooth library. To make it do usefull stuff you need to create a function of this type:

```C++
typedef void BLEcallbackType(Characteristic*, esp_ble_gatts_cb_param_t*);
```

E.g.

```C++
static uint64_t Time;
void TimeWriteCallback(Characteristic* ch, esp_ble_gatts_cb_param_t* param)
{
    ch->setData(param->write.value, param->write.len);
    Time = *(uint64_t*)&param->write.value[0];

    printf("Current time is: %08X%08X\n", (uint32_t)(Time >> 32),
                                                (uint32_t)(Time & 0xFFFFFFFF));
}
```

This function accepts write buffer and passes it to object's buffer which can be accessable via `byte* getData()`. To make it work you need to add this to `app_main`:

```C++
    std::vector <Characteristic*> TimeChars {&TimeChar};
    TimeChar.setWritehandler(TimeWriteCallback);
    Service* TimeData = new Service(0x1805, TimeChars);

    //...

    std::vector <Service*> Services {TimeData};
    BLE_Kit = ServerDevice("Name", adv_data, scan_rsp_data, adv_params, Services);
```

This will add 

```text
ServerDevice: Name
|---Service: 0x1805
|---|---Characteristic: 0x2A2B
|---|---|---TimeWriteCallback()
```

## Development

This library was created during project which monitors CAN bus and sends data via BLE to mobile app. All tests were performed on [WiFi Kit 32](https://heltec.org/project/wifi-kit-32/), most of all commits stored in a private repository, so I'll try to record all changes-features as much accurate as possible. I hope it will make your job with BLE easier.
