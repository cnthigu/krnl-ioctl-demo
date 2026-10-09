#pragma once

#include <windows.h>

bool ReadMemory ( HANDLE device, ULONG process_id, ULONG_PTR address, SIZE_T size, ULONG_PTR* out_value );
bool WriteMemory ( HANDLE device, ULONG process_id, ULONG_PTR address, ULONG_PTR value, SIZE_T size );
ULONG64 GetModuleBaseDriver ( HANDLE device, ULONG process_id, const wchar_t* module_name );
