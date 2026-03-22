/*++
Module Name:
    ptp.c

Abstract:
    High-precision time reading and adjusting logic for TimeCard.
--*/

#include "timecard.h"

NTSTATUS
TimeCardGetTime(
    _In_  PDEVICE_CONTEXT DeviceContext,
    _Out_ PTIMECARD_TIME  Time
)
{
    NTSTATUS status = STATUS_SUCCESS;
    UINT32 ctrl;
    INT i;

    WdfWaitLockAcquire(DeviceContext->RegLock, NULL);

    // 1. Request time read
    ctrl = OCP_CTRL_READ_TIME_REQ | OCP_CTRL_ENABLE;
    WRITE_REGISTER_ULONG(&DeviceContext->Regs->Ctrl, ctrl);

    // 2. Poll for completion (up to 100 iterations as in Linux driver)
    for (i = 0; i < 100; i++) {
        ctrl = READ_REGISTER_ULONG(&DeviceContext->Regs->Ctrl);
        if (ctrl & OCP_CTRL_READ_TIME_DONE) {
            break;
        }
        KeStallExecutionProcessor(1); // Small delay
    }

    if (ctrl & OCP_CTRL_READ_TIME_DONE) {
        Time->Nanoseconds = READ_REGISTER_ULONG(&DeviceContext->Regs->TimeNs);
        Time->Seconds = READ_REGISTER_ULONG(&DeviceContext->Regs->TimeSec);
    } else {
        status = STATUS_IO_TIMEOUT;
    }

    WdfWaitLockRelease(DeviceContext->RegLock);

    return status;
}

NTSTATUS
TimeCardSetTime(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ PTIMECARD_TIME  Time
)
{
    WdfWaitLockAcquire(DeviceContext->RegLock, NULL);

    WRITE_REGISTER_ULONG(&DeviceContext->Regs->AdjustNs, (UINT32)Time->Nanoseconds);
    WRITE_REGISTER_ULONG(&DeviceContext->Regs->AdjustSec, (UINT32)Time->Seconds);
    WRITE_REGISTER_ULONG(&DeviceContext->Regs->Ctrl, OCP_CTRL_ADJUST_TIME | OCP_CTRL_ENABLE);

    WdfWaitLockRelease(DeviceContext->RegLock);

    return STATUS_SUCCESS;
}
