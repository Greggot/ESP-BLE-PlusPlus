# ESP-IDF based BLE GATT Server C++ library

## What does library can do

- Initialize BLE GATT server device with name and a set of services & characteriscs
- Define callbacks for device and characteristic events
- Use notifications to send data to client

Original ESP-IDF Bluetooth library is too complicated to work with. Just look at example BLE project.  
Hence, I have written OOP-wrapper, which helps with creating BLE server. There is no more need in huge *switch-case* constructions - all of 'em are hidden inside classes implementation.

## Library structure

```text
Server
|---Service <-------------------|
|---|---Characteristic <--------|
                                |
                              GATT
```

`GATT` - abstraction for `UUID` and `Handler`. These are unique for each characteristic and service, thus class is inherited by them.

`Characteristic` - class which holds read/write callbacks, characteristic's data and notify method. So, object of this class serves as IO for some abstract purpose. It can communicate with client via incoming requests or async notifications.

`Service` - a set of characteristics united by a single abstract responcibility.

`Server` - class that holds device events, de/initializes all the classes' objects described above. Device events divide into two types: `GAP`'s and `GATTS`'s ones. `GAP` events occur on network layer: advertisement, dis/connect and so on. `GATTS` are ones that occure while working with characteristics and services. This events used to initialize these instances: give each the `UUID` and `Handler`.  

## Callbacks

`Callbacks` are the functions that are being called via reference when some event occure.

As for events, there are two types of callbacks: `GAP` and `GATTS`.

Characteristic class holds two specific `GATTS` events: `read` and `write`. They are received as a general `GATTS` event by `Server`. Requests have data in it and a handler of target, which is a characteristic. A set of `Characteristic` is being searched by given handler to call propriate callback.

All the other `GAP`-`GATTS` callbacks are executed by `Server`.

## Usage

### BLE point creation

To create BLE point you gotta write something like:

```C++
#include <Bluetooth.hpp>
using namespace BLE;

void app_main()
{
    static Characteristic characteristic = Characteristic(0xAAAA, 
        permition::Read|permition::Write, property::Read|property::Write);
    static Service service = Service(0xBBBB, {&characteristic});

    Server server("Name", { &service /*, &other, &services, &here,*/ });
    Server::Enable();
}

```

Created characteristic is being linked to service. Then service is being linked itself to the device object.

### Characteristic usage

Inside `Characteristic` there are these typedefs:

```C++
typedef std::function<void(Characteristic*, const uint16_t, const void*)> WriteCallback;
typedef std::function<void(const Characteristic*, esp_ble_gatts_cb_param_t *param)> ReadCallback;
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
    
    Server server("RemoteClock", {&TimeService});
    Server::Enable();
}
```

This will add

```text
Server:           RemoteClock
|---Service:            0xDAEA
|---|---Characteristic: 0xDCB
```

Which will set time when being written and answer system time when being read

## Development

This library was created during the CAN-BLE-Adapter-project. All tests performed on boards with [WiFi Kit 32](https://heltec.org/project/wifi-kit-32/) as a core processor.
I hope it will make your life easier.