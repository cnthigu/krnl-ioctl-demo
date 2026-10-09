#pragma once

#ifdef _KERNEL_MODE
#include <ntifs.h>
#else
#include <windows.h>
#include <winioctl.h>
#endif

#define IOCTL_ADD CTL_CODE ( FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_READ CTL_CODE ( FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_WRITE CTL_CODE ( FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define GET_MODULE CTL_CODE ( FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS )

#pragma pack( push, 1 )
typedef struct _KERNEL_READ_REQUEST
{
    ULONG process_id;
    ULONG_PTR address;
    ULONG_PTR response;
    SIZE_T size;
} KERNEL_READ_REQUEST, *PKERNEL_READ_REQUEST;

typedef struct _KERNEL_WRITE_REQUEST
{
    ULONG process_id;
    ULONG_PTR address;
    ULONG_PTR value;
    SIZE_T size;
} KERNEL_WRITE_REQUEST, *PKERNEL_WRITE_REQUEST;

typedef struct _KERNEL_MODULE_REQUEST
{
    ULONG process_id;
    WCHAR module_name[256];
    ULONG64 base_address;
} KERNEL_MODULE_REQUEST, *PKERNEL_MODULE_REQUEST;
#pragma pack( pop )
