/*++
Module Name:
    driver.c

Abstract:
    Core driver entry points and device initialization for TimeCard.
--*/

#include "timecard.h"

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    WDF_DRIVER_CONFIG_INIT(&config, DeviceEvtDeviceAdd);

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             WDF_NO_HANDLE);

    return status;
}

NTSTATUS
DeviceEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDFDEVICE device;
    NTSTATUS status;
    PDEVICE_CONTEXT deviceContext;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;

    UNREFERENCED_PARAMETER(Driver);

    //
    // Set PnP and Power callbacks
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = DeviceEvtPrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = DeviceEvtReleaseHardware;
    pnpPowerCallbacks.EvtDeviceD0Entry = DeviceEvtD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit = DeviceEvtD0Exit;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Create the device
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    deviceContext = DeviceGetContext(device);
    deviceContext->Device = device;

    //
    // Create the default IO queue for IOCTLs
    //
    WDF_IO_QUEUE_CONFIG ioQueueConfig;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);
    ioQueueConfig.EvtIoDeviceControl = DeviceEvtIoDeviceControl;

    status = WdfIoQueueCreate(device, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Create a wait lock for register access
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&deviceAttributes);
    deviceAttributes.ParentObject = device;
    status = WdfWaitLockCreate(&deviceAttributes, &deviceContext->RegLock);

    return status;
}

NTSTATUS
DeviceEvtPrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
)
{
    PDEVICE_CONTEXT deviceContext = DeviceGetContext(Device);
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;
    ULONG i;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN foundBar0 = FALSE;

    UNREFERENCED_PARAMETER(ResourcesRaw);

    for (i = 0; i < WdfCmResourceListGetCount(ResourcesTranslated); i++) {
        descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

        switch (descriptor->Type) {
            case CmResourceTypeMemory:
                if (!foundBar0) {
                    deviceContext->Bar0Base = mmMapIoSpace(descriptor->u.Memory.Start,
                                                            descriptor->u.Memory.Length,
                                                            MmNonCached);
                    deviceContext->Bar0Length = descriptor->u.Memory.Length;
                    foundBar0 = TRUE;
                    
                    // Setup register pointers
                    deviceContext->Regs = (POCP_REG)((PUCHAR)deviceContext->Bar0Base + TIMECARD_REG_OFFSET);
                    deviceContext->Tod = (PTOD_REG)((PUCHAR)deviceContext->Bar0Base + TIMECARD_TOD_OFFSET);
                }
                break;

            case CmResourceTypeInterrupt:
                // MSI/MSI-X handling will be added later
                break;

            default:
                break;
        }
    }

    if (!foundBar0) {
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    return status;
}

NTSTATUS
DeviceEvtReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated
)
{
    PDEVICE_CONTEXT deviceContext = DeviceGetContext(Device);

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    if (deviceContext->Bar0Base) {
        MmUnmapIoSpace(deviceContext->Bar0Base, deviceContext->Bar0Length);
        deviceContext->Bar0Base = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DeviceEvtD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(PreviousState);
    return STATUS_SUCCESS;
}

NTSTATUS
DeviceEvtD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(TargetState);
    return STATUS_SUCCESS;
}
