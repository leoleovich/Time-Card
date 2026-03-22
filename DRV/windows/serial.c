/*++
Module Name:
    serial.c

Abstract:
    Child device enumeration for the TimeCard serial ports.
--*/

#include "timecard.h"

//
// Child device hardware IDs
//
#define TIMECARD_UART_HWID L"TIMECARD\\UART\0"

NTSTATUS
TimeCardEnumerateSerialPorts(
    _In_ PDEVICE_CONTEXT DeviceContext
)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE pdo;
    PWDFDEVICE_INIT pdoInit = NULL;
    DECLARE_UNICODE_STRING_SIZE(deviceId, 64);
    DECLARE_UNICODE_STRING_SIZE(instanceId, 64);
    ULONG i;

    struct {
        ULONG Offset;
        PCWSTR Name;
    } ports[] = {
        { TIMECARD_UART_GNSS_OFFSET,  L"GNSS"  },
        { TIMECARD_UART_GNSS2_OFFSET, L"GNSS2" },
        { TIMECARD_UART_MAC_OFFSET,   L"MAC"   },
        { TIMECARD_UART_NMEA_OFFSET,  L"NMEA"  }
    };

    for (i = 0; i < 4; i++) {
        pdoInit = WdfPdoInitAllocate(DeviceContext->Device);
        if (!pdoInit) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        // Set hardware ID
        RtlUnicodeStringPrintf(&deviceId, L"TIMECARD\\%s", ports[i].Name);
        status = WdfPdoInitAddHardwareID(pdoInit, &deviceId);
        if (!NT_SUCCESS(status)) {
            WdfDeviceInitFree(pdoInit);
            break;
        }

        // Set device description
        WdfDeviceInitSetDeviceType(pdoInit, FILE_DEVICE_SERIAL_PORT);

        // Setup context
        WDF_OBJECT_ATTRIBUTES deviceAttributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, SERIAL_PORT_CONTEXT);
        status = WdfDeviceCreate(&pdoInit, &deviceAttributes, &pdo);
        if (!NT_SUCCESS(status)) {
            WdfDeviceInitFree(pdoInit);
            break;
        }

        PSERIAL_PORT_CONTEXT serialContext = SerialPortGetContext(pdo);
        serialContext->Parent = DeviceContext->Device;
        serialContext->Offset = ports[i].Offset;
        serialContext->Name = ports[i].Name;

        // Add to child list
        status = WdfChildListAddChildToFixedList(WdfDeviceGetDefaultChildList(DeviceContext->Device), pdoInit);
        if (!NT_SUCCESS(status)) {
            // WdfDeviceCreate already took ownership of pdoInit
            break;
        }
    }

    return status;
}
