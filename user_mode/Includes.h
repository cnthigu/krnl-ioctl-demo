#pragma once
#include <iostream>
#include <windows.h>
#include <winioctl.h>
#include <tlhelp32.h>
#include <stdio.h>

#define IOCTL_ADD   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define GET_MODULE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

#pragma pack(push, 1)
typedef struct _KERNEL_READ_REQUEST {
    ULONG     ProcessId;
    ULONG_PTR Address;
    ULONG_PTR Response;
    SIZE_T    Size;
} KERNEL_READ_REQUEST, * PKERNEL_READ_REQUEST;

typedef struct _KERNEL_WRITE_REQUEST {
    ULONG     ProcessId;
    ULONG_PTR Address;
    ULONG_PTR Value;
    SIZE_T    Size;
} KERNEL_WRITE_REQUEST, * PKERNEL_WRITE_REQUEST;

typedef struct _KERNEL_MODULE_REQUEST {
    ULONG   ProcessId;
    WCHAR   ModuleName[256];   // entrada:
    ULONG64 BaseAddress;       // saída
} KERNEL_MODULE_REQUEST, * PKERNEL_MODULE_REQUEST;
#pragma pack(pop)

inline bool ReadMemory(HANDLE hDevice, ULONG ProcessId, ULONG_PTR Address, SIZE_T Size, ULONG_PTR* pOutValue)
{
    KERNEL_READ_REQUEST req = { 0 };
    req.ProcessId = ProcessId;
    req.Address = Address;
    req.Size = Size;

    DWORD cbReturned = 0;
    BOOL ok = DeviceIoControl(hDevice, IOCTL_READ,
        &req, sizeof(req),
        &req, sizeof(req),
        &cbReturned,
        nullptr);

    if (ok && pOutValue)
        *pOutValue = req.Response;
    return ok != FALSE;
}

inline bool WriteMemory(HANDLE hDevice, ULONG ProcessId, ULONG_PTR Address, ULONG_PTR Value, SIZE_T Size)
{
    KERNEL_WRITE_REQUEST req = { 0 };
    req.ProcessId = ProcessId;
    req.Address = Address;
    req.Value = Value;
    req.Size = Size;

    DWORD cbReturned = 0;
    return DeviceIoControl(hDevice, IOCTL_WRITE,
        &req, sizeof(req),
        &req, sizeof(req),
        &cbReturned, nullptr) != FALSE;
}

inline ULONG64 GetModuleBaseDriver(HANDLE hDevice, ULONG ProcessId, const wchar_t* moduleName)
{
    if (!moduleName)
        return 0;

    KERNEL_MODULE_REQUEST req = { 0 };
    req.ProcessId = ProcessId;
    wcsncpy_s(req.ModuleName, _countof(req.ModuleName), moduleName, _TRUNCATE);

    DWORD cbReturned = 0;

    BOOL ok = DeviceIoControl(hDevice, GET_MODULE,
        &req, sizeof(req),
        &req, sizeof(req),
        &cbReturned,
        nullptr);

    if (!ok || cbReturned < sizeof(req))
        return 0;

    return req.BaseAddress;
}

static ULONG GetPidByName(const wchar_t* processName)
{
    ULONG pid = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(hSnapshot, &pe))
    {
        do
        {
            if (_wcsicmp(pe.szExeFile, processName) == 0)
            {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return pid;
}