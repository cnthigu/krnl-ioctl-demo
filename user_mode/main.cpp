#include "Includes.h"

int main()
{
    HANDLE hDevice = CreateFileA("\\\\.\\SimpleDriver",
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printf("[!] CreateFile failed (%lu). Driver loaded?\n", GetLastError());
        system("pause");
        return 1;
    }

    ULONG pid = GetPidByName(L"notepad.exe");

    if (pid == 0)
    {
        printf("[!] notepad.exe process not found.\n");
        CloseHandle(hDevice);
        return 1;
    }

    ULONG64 baseKernel = GetModuleBaseDriver(hDevice, pid, L"notepad.exe");

    if (baseKernel == 0)
    {
        printf("[!] PID falhou\n");
        return 2;
    }

    ULONG_PTR addr = 0x0000000;  // test addr

    ULONG_PTR value = 0;

    if (ReadMemory(hDevice, pid, addr, sizeof(value), &value))
    {
        printf("Value: %llu\n", value);
    }

    system("pause");
    CloseHandle(hDevice);

    return 0;
}