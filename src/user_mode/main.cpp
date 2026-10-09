#include "driver_client.h"

#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

static ULONG GetPidByName ( const wchar_t* process_name )
{
    ULONG process_id = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot ( TH32CS_SNAPPROCESS, 0 );

    if ( snapshot == INVALID_HANDLE_VALUE )
    {
        return 0;
    }

    PROCESSENTRY32W process_entry = { sizeof ( process_entry ) };
    if ( Process32FirstW ( snapshot, &process_entry ) )
    {
        do
        {
            if ( _wcsicmp ( process_entry.szExeFile, process_name ) == 0 )
            {
                process_id = process_entry.th32ProcessID;
                break;
            }
        } while ( Process32NextW ( snapshot, &process_entry ) );
    }
    CloseHandle ( snapshot );
    return process_id;
}

int main ()
{
    HANDLE device = CreateFileA ( "\\\\.\\SimpleDriver", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL, nullptr );

    if ( device == INVALID_HANDLE_VALUE )
    {
        printf ( "[!] CreateFile failed (%lu). Driver loaded?\n", GetLastError () );
        system ( "pause" );
        return 1;
    }

    ULONG process_id = GetPidByName ( L"notepad.exe" );

    if ( process_id == 0 )
    {
        printf ( "[!] notepad.exe process not found.\n" );
        CloseHandle ( device );
        return 1;
    }

    ULONG64 module_base = GetModuleBaseDriver ( device, process_id, L"notepad.exe" );

    if ( module_base == 0 )
    {
        printf ( "[!] PID falhou\n" );
        return 2;
    }

    ULONG_PTR address = 0x0000000;

    ULONG_PTR value = 0;

    if ( ReadMemory ( device, process_id, address, sizeof ( value ), &value ) )
    {
        printf ( "Value: %llu\n", value );
    }

    system ( "pause" );
    CloseHandle ( device );

    return 0;
}