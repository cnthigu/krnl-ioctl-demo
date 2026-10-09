#include "process.h"
#include "native.h"

NTSTATUS ReadProcessMemory ( PEPROCESS process, PVOID source_address, PVOID target_address, SIZE_T size )
{
    PEPROCESS source_process = process;
    PEPROCESS target_process = PsGetCurrentProcess ();

    SIZE_T bytes_written = 0;

    NTSTATUS status = MmCopyVirtualMemory ( source_process, source_address, target_process, target_address, size,
                                            KernelMode, &bytes_written );
    return status;
}

NTSTATUS WriteProcessMemory ( PEPROCESS process, PVOID source_address, PVOID target_address, SIZE_T size )
{
    PEPROCESS source_process = PsGetCurrentProcess ();
    PEPROCESS target_process = process;
    SIZE_T bytes_written = 0;
    NTSTATUS status = MmCopyVirtualMemory ( source_process, source_address, target_process, target_address, size,
                                            KernelMode, &bytes_written );
    return status;
}

ULONG64 GetModuleBaseX64 ( PEPROCESS process, UNICODE_STRING module_name )
{
    PPEB peb = (PPEB)PsGetProcessPeb ( process );

    if ( !peb )
    {
        return 0;
    }

    KAPC_STATE state;
    KeStackAttachProcess ( process, &state );

    PPEB_LDR_DATA ldr = (PPEB_LDR_DATA)peb->Ldr;
    if ( !ldr )
    {
        KeUnstackDetachProcess ( &state );
        return 0;
    }

    for ( PLIST_ENTRY list = (PLIST_ENTRY)ldr->ModuleListLoadOrder.Flink; list != &ldr->ModuleListLoadOrder;
          list = (PLIST_ENTRY)list->Flink )
    {
        PLDR_DATA_TABLE_ENTRY entry = CONTAINING_RECORD ( list, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList );

        if ( RtlCompareUnicodeString ( &entry->BaseDllName, &module_name, TRUE ) == 0 )
        {
            ULONG64 base_address = (ULONG64)entry->DllBase;
            KeUnstackDetachProcess ( &state );
            return base_address;
        }
    }

    KeUnstackDetachProcess ( &state );
    return 0;
}