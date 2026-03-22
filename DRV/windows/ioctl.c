/*++
Module Name:
    ioctl.c

Abstract:
    IOCTL handling for the TimeCard driver.
--*/

#include "timecard.h"

// Prototype for the internal function
NTSTATUS TimeCardGetTime(_In_ PDEVICE_CONTEXT DeviceContext, _Out_ PTIMECARD_TIME Time);

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL DeviceEvtIoDeviceControl;

NTSTATUS
DeviceEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_CONTEXT deviceContext = DeviceGetContext(WdfIoQueueGetDevice(Queue));
    size_t bytesReturned = 0;

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    switch (IoControlCode) {
        case IOCTL_TIMECARD_GET_TIME:
            {
                TIMECARD_TIME* outputTime;
                status = WdfRequestRetrieveOutputBuffer(Request, sizeof(TIMECARD_TIME), (PVOID*)&outputTime, NULL);
                if (NT_SUCCESS(status)) {
                    status = TimeCardGetTime(deviceContext, outputTime);
                    if (NT_SUCCESS(status)) {
                        bytesReturned = sizeof(TIMECARD_TIME);
                    }
                }
            }
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    WdfRequestCompleteWithInformation(Request, status, bytesReturned);
    return status;
}
