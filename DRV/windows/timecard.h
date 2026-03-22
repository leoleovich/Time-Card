/*++
Module Name:
    timecard.h

Abstract:
    Header file for the TimeCard Windows Driver.
    Contains register definitions and device context.
--*/

#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>

//
// PCI IDs
//
#define PCI_VENDOR_ID_FACEBOOK   0x1d9b
#define PCI_VENDOR_ID_CELESTICA  0x18d4
#define PCI_VENDOR_ID_OROLIA     0x1ad7

#define PCI_DEVICE_ID_FACEBOOK_TIMECARD  0x0400
#define PCI_DEVICE_ID_CELESTICA_TIMECARD 0x1008
#define PCI_DEVICE_ID_OROLIA_ARTCARD     0xa000

//
// Register Offsets (from ptp_ocp.c rev1)
//
#define TIMECARD_REG_OFFSET         0x01000000
#define TIMECARD_TOD_OFFSET         0x01050000
#define TIMECARD_I2C_CTRL_OFFSET    0x00150000
#define TIMECARD_UART_GNSS_OFFSET   (0x00160000 + 0x1000)
#define TIMECARD_UART_GNSS2_OFFSET  (0x00170000 + 0x1000)
#define TIMECARD_UART_MAC_OFFSET    (0x00180000 + 0x1000)
#define TIMECARD_UART_NMEA_OFFSET   (0x00190000 + 0x1000)

//
// OCP Core Registers
//
typedef struct _OCP_REG {
    UINT32 Ctrl;
    UINT32 Status;
    UINT32 Select;
    UINT32 Version;
    UINT32 TimeNs;
    UINT32 TimeSec;
    UINT32 Reserved0[2];
    UINT32 AdjustNs;
    UINT32 AdjustSec;
    UINT32 Reserved1[2];
    UINT32 OffsetNs;
    UINT32 OffsetWindowNs;
    UINT32 Reserved2[2];
    UINT32 DriftNs;
    UINT32 DriftWindowNs;
    UINT32 Reserved3[6];
    UINT32 ServoOffsetP;
    UINT32 ServoOffsetI;
    UINT32 ServoDriftP;
    UINT32 ServoDriftI;
    UINT32 StatusOffset;
    UINT32 StatusDrift;
} OCP_REG, *POCP_REG;

#define OCP_CTRL_ENABLE         0x00000001
#define OCP_CTRL_ADJUST_TIME    0x00000002
#define OCP_CTRL_READ_TIME_REQ  0x40000000
#define OCP_CTRL_READ_TIME_DONE 0x80000000

//
// TOD (Time of Day) Registers
//
typedef struct _TOD_REG {
    UINT32 Ctrl;
    UINT32 Status;
    UINT32 UartPolarity;
    UINT32 Version;
    UINT32 AdjSec;
    UINT32 Reserved0[3];
    UINT32 UartBaud;
    UINT32 Reserved1[3];
    UINT32 UtcStatus;
    UINT32 Leap;
    UINT32 Reserved2[2];
    UINT32 GnssStatus;
    UINT32 NumSat;
} TOD_REG, *PTOD_REG;

//
// Device Context
//
typedef struct _DEVICE_CONTEXT {
    WDFDEVICE Device;
    
    // Mapped BAR 0
    PVOID Bar0Base;
    ULONG Bar0Length;

    // Register pointers
    POCP_REG Regs;
    PTOD_REG Tod;

    // Serialization
    WDFWAITLOCK RegLock;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

//
// Child Device Context (Serial)
//
typedef struct _SERIAL_PORT_CONTEXT {
    WDFDEVICE Parent;
    ULONG Offset;
    PCWSTR Name;
} SERIAL_PORT_CONTEXT, *PSERIAL_PORT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SERIAL_PORT_CONTEXT, SerialPortGetContext)

//
// Prototypes
//
EVT_WDF_DRIVER_DEVICE_ADD DeviceEvtDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE DeviceEvtPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE DeviceEvtReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY DeviceEvtD0Entry;
EVT_WDF_DEVICE_D0_EXIT DeviceEvtD0Exit;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL DeviceEvtIoDeviceControl;

NTSTATUS TimeCardEnumerateSerialPorts(_In_ PDEVICE_CONTEXT DeviceContext);

// IOCTL Definitions
#define IOCTL_TIMECARD_GET_TIME \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _TIMECARD_TIME {
    UINT64 Seconds;
    UINT32 Nanoseconds;
} TIMECARD_TIME, *PTIMECARD_TIME;
