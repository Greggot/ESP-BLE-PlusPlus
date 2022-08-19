#pragma once
#include <debug.hpp>

#define BLE_SERVICES_PRINTF
#define BLE_CHARACTERISTICS_PRINTF
#define BLE_INPUT_PRINTF

#ifdef BLE_INPUT_PRINTF
    #define MAX_OUTPUT_AMOUNT 40
#endif

#define BLE Bluetooth

namespace Perm
{
    enum bit
    {
        Read            = 1,
        ReadEncrypted   = (1 << 1),
        ReadEncrMitm    = (1 << 2),
        Write           = (1 << 4),
        WriteEncrypted  = (1 << 5),
        WriteEncrMitm   = (1 << 6),
        WriteSigned     = (1 << 7),
        WriteSignMitm   = (1 << 8),
        ReadAuth        = (1 << 9),
        WriteAuth       = (1 << 10),
    };
}
namespace Prop
{
    enum bit
    {
        Broadcast   = 1,
        Read        = (1 << 1),
        WriteNr     = (1 << 2),
        Write       = (1 << 3),
        Notify      = (1 << 4),
        Indicate    = (1 << 5),
        Auth        = (1 << 6),
        ExtProp     = (1 << 7),
    };
}