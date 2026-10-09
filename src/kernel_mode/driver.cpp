#include "native.h"
#include "process.h"
#include "../shared/ioctl.h"

#define DEVICE_NAME L"\\Device\\SimpleDriver"
#define SYMLINK_NAME L"\\DosDevices\\SimpleDriver"

NTSTATUS CreateClose ( PDEVICE_OBJECT device_object, PIRP irp )
{
    UNREFERENCED_PARAMETER ( device_object );

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest ( irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

NTSTATUS DeviceControl ( PDEVICE_OBJECT device_object, PIRP irp )
{
    UNREFERENCED_PARAMETER ( device_object );

    PIO_STACK_LOCATION irp_stack = IoGetCurrentIrpStackLocation ( irp );
    PVOID system_buffer = irp->AssociatedIrp.SystemBuffer;

    ULONG input_buffer_length = irp_stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG output_buffer_length = irp_stack->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG io_control_code = irp_stack->Parameters.DeviceIoControl.IoControlCode;

    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    ULONG bytes_returned = 0;

    switch ( io_control_code )
    {
    case IOCTL_ADD:
    {
        if ( input_buffer_length >= sizeof ( int ) && output_buffer_length >= sizeof ( int ) && system_buffer != NULL )
        {
            int* value = (int*)system_buffer;
            *value = *value + 1;
            bytes_returned = sizeof ( int );
            status = STATUS_SUCCESS;
            DbgPrint ( "[+] IOCTL_ADD: %d -> %d\n", *value - 1, *value );
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }

    case IOCTL_READ:
    {
        if ( input_buffer_length >= sizeof ( KERNEL_READ_REQUEST ) &&
             output_buffer_length >= sizeof ( KERNEL_READ_REQUEST ) && system_buffer != NULL )
        {
            PKERNEL_READ_REQUEST read_request = (PKERNEL_READ_REQUEST)system_buffer;
            PEPROCESS process = NULL;

            status = PsLookupProcessByProcessId ( (HANDLE)(ULONG_PTR)read_request->process_id, &process );
            if ( !NT_SUCCESS ( status ) )
            {
                DbgPrint ( "[+] READ: PID %lu not found 0x%08X\n", read_request->process_id, status );
                break;
            }

            status = ReadProcessMemory ( process, (PVOID)read_request->address, (PVOID)&read_request->response,
                                         read_request->size );

            ObfDereferenceObject ( process );

            if ( NT_SUCCESS ( status ) )
            {
                bytes_returned = sizeof ( KERNEL_READ_REQUEST );
                DbgPrint ( "[+] READ: PID %lu addr %p -> %llu bytes\n", read_request->process_id,
                           (void*)read_request->address, (unsigned long long)read_request->size );
            }
            else
            {
                DbgPrint ( "[+] READ: MmCopyVirtualMemory failed 0x%08X\n", status );
            }
        }
        else
            status = STATUS_BUFFER_TOO_SMALL;
        break;
    }

    case IOCTL_WRITE:
    {
        if ( input_buffer_length >= sizeof ( KERNEL_WRITE_REQUEST ) &&
             output_buffer_length >= sizeof ( KERNEL_WRITE_REQUEST ) && system_buffer != NULL )
        {
            PKERNEL_WRITE_REQUEST write_request = (PKERNEL_WRITE_REQUEST)system_buffer;
            PEPROCESS process = NULL;

            status = PsLookupProcessByProcessId ( (HANDLE)(ULONG_PTR)write_request->process_id, &process );
            if ( !NT_SUCCESS ( status ) )
            {
                DbgPrint ( "[+] WRITE: PID %lu not found 0x%08X\n", write_request->process_id, status );
                break;
            }

            status = WriteProcessMemory ( process, (PVOID)&write_request->value, (PVOID)write_request->address,
                                          write_request->size );

            ObfDereferenceObject ( process );

            if ( NT_SUCCESS ( status ) )
            {
                bytes_returned = sizeof ( KERNEL_WRITE_REQUEST );
                DbgPrint ( "[+] WRITE: PID %lu addr %p %llu bytes\n", write_request->process_id,
                           (void*)write_request->address, (unsigned long long)write_request->size );
            }
            else
            {
                DbgPrint ( "[+] WRITE: failed 0x%08X (demo uses fake addr, may be read-only)\n", status );
            }
        }
        else
            status = STATUS_BUFFER_TOO_SMALL;
        break;
    }

    case GET_MODULE:
    {
        if ( input_buffer_length >= sizeof ( KERNEL_MODULE_REQUEST ) &&
             output_buffer_length >= sizeof ( KERNEL_MODULE_REQUEST ) && system_buffer != NULL )
        {
            PKERNEL_MODULE_REQUEST module_request = (PKERNEL_MODULE_REQUEST)system_buffer;
            PEPROCESS process = NULL;

            module_request->module_name[RTL_NUMBER_OF ( module_request->module_name ) - 1] = L'\0';

            status = PsLookupProcessByProcessId ( (HANDLE)(ULONG_PTR)module_request->process_id, &process );
            if ( !NT_SUCCESS ( status ) )
            {
                DbgPrint ( "[+] GET_MODULE: PID %lu not found 0x%08X\n", module_request->process_id, status );
                break;
            }

            UNICODE_STRING module_name;
            RtlInitUnicodeString ( &module_name, module_request->module_name );

            ULONG64 base = GetModuleBaseX64 ( process, module_name );

            ObfDereferenceObject ( process );

            module_request->base_address = base;
            bytes_returned = sizeof ( KERNEL_MODULE_REQUEST );

            if ( base != 0 )
            {
                status = STATUS_SUCCESS;
                DbgPrint ( "[+] GET_MODULE: PID %lu module %ws base 0x%llX\n", module_request->process_id,
                           module_request->module_name, (unsigned long long)base );
            }
            else
            {
                status = STATUS_NOT_FOUND;
                DbgPrint ( "[+] GET_MODULE: module %ws not found in PID %lu\n", module_request->module_name,
                           module_request->process_id );
            }
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }
    irp->IoStatus.Status = status;
    irp->IoStatus.Information = bytes_returned;
    IoCompleteRequest ( irp, IO_NO_INCREMENT );
    return status;
}

VOID UnloadDriver ( PDRIVER_OBJECT driver_object )
{
    UNICODE_STRING symbolic_link;
    RtlInitUnicodeString ( &symbolic_link, SYMLINK_NAME );
    IoDeleteSymbolicLink ( &symbolic_link );

    if ( driver_object->DeviceObject != NULL )
    {
        IoDeleteDevice ( driver_object->DeviceObject );
    }

    DbgPrint ( "[+] UnloadDriver: device removed\n" );
}

NTSTATUS DriverInitialize ( PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path )
{
    UNREFERENCED_PARAMETER ( registry_path );

    driver_object->MajorFunction[IRP_MJ_CREATE] = CreateClose;
    driver_object->MajorFunction[IRP_MJ_CLOSE] = CreateClose;
    driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
    driver_object->DriverUnload = UnloadDriver;

    UNICODE_STRING device_name, symbolic_link;
    PDEVICE_OBJECT device = NULL;

    RtlInitUnicodeString ( &device_name, DEVICE_NAME );

    NTSTATUS status = IoCreateDevice ( driver_object, 0, &device_name, FILE_DEVICE_UNKNOWN, 0, TRUE, &device );

    if ( !NT_SUCCESS ( status ) )
        return status;

    RtlInitUnicodeString ( &symbolic_link, SYMLINK_NAME );
    status = IoCreateSymbolicLink ( &symbolic_link, &device_name );

    if ( !NT_SUCCESS ( status ) )
    {
        IoDeleteDevice ( device );
        return status;
    }

    device->Flags |= DO_BUFFERED_IO;
    device->Flags &= ~DO_DEVICE_INITIALIZING;

    DbgPrint ( "[+] Driver loaded\n" );

    return STATUS_SUCCESS;
}

extern "C" NTSTATUS DriverEntry ( PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path )
{
    if ( !driver_object )
    {
        UNICODE_STRING driver_name;
        RtlInitUnicodeString ( &driver_name, L"\\Driver\\SimpleDriver" );
        return IoCreateDriver ( &driver_name, &DriverInitialize );
    }
    return DriverInitialize ( driver_object, registry_path );
}