#pragma once

#include <ntifs.h>

NTSTATUS ReadProcessMemory ( PEPROCESS process, PVOID source_address, PVOID target_address, SIZE_T size );
NTSTATUS WriteProcessMemory ( PEPROCESS process, PVOID source_address, PVOID target_address, SIZE_T size );
ULONG64 GetModuleBaseX64 ( PEPROCESS process, UNICODE_STRING module_name );
