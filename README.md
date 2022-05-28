# ESP-IDF based BLE GATT Server C++ library

## What does library can do

- Initialize BLE GATT server device with name and a set of services & characteriscs
- Define callbacks for device and characteristic events
- Use notifications to send data to client

Original ESP-IDF Bluetooth library is too complicated to work with: almoust no abstraction mechanism is present. Thus, this library make sure to enable `bluedroid` and advertising by itself; let user write independed callback functions with no need to implement one giant `switch-case` construction.

## Library structure

```text
ServerDevice
|---Service <-------------------|
|---|---Characteristic <--------|
                                |
                            GATTinstance
```

`GATTinstance` - class which represents `UUID` and `Handler`. These are unique for each characteristic and service, thus instance is inherited by them.

`Characteristic` - class which holds read/write callbacks, characteristic's data and notify method. So, object of this class serves as IO for some abstract purpose. It can communicate with client via incoming requests or async notifications.

`Service` - a set of characteristics united by a single abstract responcibility.

`ServerDevice` - class that holds device events, de/initializes all the classes' objects described above. Device events divide into two types: `GAP`'s and `GATTS`'s ones. `GAP` events occur on network layer: advertisement, dis/connect and so on. `GATTS` are ones that occure while working with characteristics and services. This events used to initialize these instances: give each the `UUID` and `Handler`.  

## Callbacks

`Callbacks` are the functions that are being called via reference when some event occure.

As for events, there are two types of callbacks: `GAP` and `GATTS`.

Characteristic class holds two specific `GATTS` events: `read` and `write`. They are received as a general `GATTS` event by `ServerDevice`. Requests have data in it and a handler of target, which is a characteristic. A set of `Characteristic` is being searched by given handler to call propriate callback.

All the other `GAP`-`GATTS` callbacks are executed by `ServerDevice`.

## Usage

### BLE point creation

To create BLE point you gotta write something like:

```C++
#include <Bluetooth.hpp>

void app_main()
{
    static Characteristic characteristic = Characteristic(0xAAAA, 
        Perm::Read|Perm::Write, Prop::Read|Prop::Write);
    static Service service = Service(0xBBBB, {&characteristic});

    ServerDevice server("Name", { &service /*, &other, &services, &here,*/ });
    ServerDevice::Enable();
}

```

Created characteristic is being linked to service. Then service is being linked itself to the device object.

### Characteristic usage

Inside `Characteristic` there are these typedefs:

```C++
typedef void (*WriteCallback)(Characteristic*, const uint16_t, const void*);
typedef void (*ReadCallback)(const Characteristic*, esp_ble_gatts_cb_param_t *param);
```

You can create a function and then link it to the characteristic, or use lambda functions:

```C++
void TimeReadCallback(const Characteristic* ch, esp_ble_gatts_cb_param_t *param)
{   
    uint64_t T = getSystemEpochTime();
    ch->Responce(&T, sizeof(T), param);
}

int app_main()
{
    static Characteristic TimeControl(0xDCB, Perm::Write, Prop::Write);
    TimeControl.setWriteCallback([](Characteristic* ch, const uint16_t len, const void* value){
        if(len != sizeof(uint64_t))
            return;
        setEpochTime(*(uint64_t*)value);
    });
    TimeControl.setReadCallback(TimeReadCallback);
    /*...*/
    static Service TimeService = Service(0xDAEA, {&TimeControl});
    
    ServerDevice server("RemoteClock", {&TimeService});
    ServerDevice::Enable();
}
```

This will add

```text
ServerDevice:           RemoteClock
|---Service:            0xDAEA
|---|---Characteristic: 0xDCB
```

Which will set time when being written and answer system time when being read

## Development

This library was created during project which monitors CAN bus and sends data via BLE to mobile app. All tests were performed on [WiFi Kit 32](https://heltec.org/project/wifi-kit-32/), most of all commits stored in a private repository, so I'll try to record all changes-features as much accurate as possible. I hope it will make your life easier when using BLE on ESP32.
