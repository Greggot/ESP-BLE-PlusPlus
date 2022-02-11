# ESP-IDF based BLE GATT Server C++ library

## What does library can do

- Initialize BLE GATT server device with name and a set of services & characteriscs
- Define callbacks for device and characteristic events
- Use notifications to send data to client

## Library structure

```text
ServerDevice - BLE port with name and address
|---Service - Abstract collection of 'values'------------|
|---|---Characteristic - 'Value' with callbacks----------|
                                                         |
                                GATTinstance - Service and Characteristic common parts
```

Original ESP-IDF Bluetooth library is too complicated to work with: almoust no abstraction mechanism is present. Thus, this library make sure to enable `bluedroid` and advertising by itself; let user write independed callback functions with no need to implement one giant `switch-case` construction.

### Hierarchy

`ServiceDevice` class has an `std::map` for each service and characteristic added to it. It associates a 16-bit `handler` with `Service` and `Characteristic` objects. Each `Service` have a `std::vector` of its characteristics, making it possible to accept all abstractly united 'values'.

*upd.* `GATTinstance` - class inherited by `Service` and `Characteristic` that takes care of `UUID`, `GATTinterface`, `handler` and console output of these parameters.

### Callbacks

Each `Characteristic` has `ReadHandler` and `WriteHandler`. These are set via `public` setters, which tell object to call sertain function when one of {read, write} events happen.

`ServerDevice` has an array of pointers to functions, which'll be called if they're non empty. It is in this way because class itself is a wrap around static function which is called each time any GATT event occures. This array's elements are set by user and is called via class function. To make it happen user stil needs to do this:

```C++
void app_main()
{
    // Overall services
    std::vector<Service*> Services;
    
    // Define a device with name and advertising parameters
    static ServerDevice BLE_Kit = ServerDevice("Name", adv_data, scan_rsp_data, adv_params, Services);

    // Pass object's method as a callback for every GATT server
    esp_ble_gatts_register_callback([](esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param){
        BLE_Kit.HandleEvent(event, gatts_if, param);
    });
    //Register callback main purpose of which is to start advertising after disconnect
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_app_register(0);
}

```

**NOTE**:
*Due to poorly design of ESP-IDF lib there's a **specific order** of functions to
be called one after another*. Example shows this order.

## Characteristic usage

Consider

```C++
static Characteristic TimeChar = Characteristic(0x2A2B, ESP_GATT_PERM_WRITE | ESP_GATT_PERM_READ, ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ);
```

global visible to different functions object. It has an UUID of `0x2A2B` and set of permitions and properties, defined by idf bluetooth library. To make it do usefull stuff you need to create a function of this type:

```C++
typedef void GATTScallbackType(Characteristic*, esp_ble_gatts_cb_param_t*);
```

### Write callback

```C++
uint64_t Time = 0;
void WriteTime(Characteristic* ch, esp_ble_gatts_cb_param_t* param)
{
    ch->setData(param->write.value, param->write.len);
    Time = *(uint64_t*)&param->write.value[0];

    printf("Current time is: %08X%08X\n", (uint32_t)(Time >> 32),
                                                (uint32_t)(Time & 0xFFFFFFFF));
}
```

### Read callback

```C++
void ReadTime(Characteristic* ch, esp_ble_gatts_cb_param_t* param)
{
    printf("Time is being read...\n)");
    ch->Responce(&Time, sizeof(uint64_t), param);
}
```

To make it work you need to add this to `app_main`:

```C++
    TimeChar.setWriteCallback(WriteTime);
    TimeChar.setReadCallback(ReadTime);
    Service* TimeData = new Service(0x1805, {&TimeChars});

    /*...*/

    BLE_Kit = ServerDevice("Name", adv_data, scan_rsp_data, adv_params, {&TimeData /*...*/});
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
