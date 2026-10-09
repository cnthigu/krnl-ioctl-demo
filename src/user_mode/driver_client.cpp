#include "driver_client.h"
#include "../shared/ioctl.h"

#include <wchar.h>

bool ReadMemory ( HANDLE device, ULONG process_id, ULONG_PTR address, SIZE_T size, ULONG_PTR* out_value )
{
    KERNEL_READ_REQUEST request = { 0 };
    request.process_id = process_id;
    request.address = address;
    request.size = size;

    DWORD bytes_returned = 0;
    BOOL ok = DeviceIoControl ( device, IOCTL_READ, &request, sizeof ( request ), &request, sizeof ( request ),
                                &bytes_returned, nullptr );

    if ( ok && out_value )
        *out_value = request.response;
    return ok != FALSE;
}

bool WriteMemory ( HANDLE device, ULONG process_id, ULONG_PTR address, ULONG_PTR value, SIZE_T size )
{
    KERNEL_WRITE_REQUEST request = { 0 };
    request.process_id = process_id;
    request.address = address;
    request.value = value;
    request.size = size;

    DWORD bytes_returned = 0;
    return DeviceIoControl ( device, IOCTL_WRITE, &request, sizeof ( request ), &request, sizeof ( request ),
                             &bytes_returned, nullptr ) != FALSE;
}

ULONG64 GetModuleBaseDriver ( HANDLE device, ULONG process_id, const wchar_t* module_name )
{
    if ( !module_name )
        return 0;

    KERNEL_MODULE_REQUEST request = { 0 };
    request.process_id = process_id;
    wcsncpy_s ( request.module_name, _countof ( request.module_name ), module_name, _TRUNCATE );

    DWORD bytes_returned = 0;

    BOOL ok = DeviceIoControl ( device, GET_MODULE, &request, sizeof ( request ), &request, sizeof ( request ),
                                &bytes_returned, nullptr );

    if ( !ok || bytes_returned < sizeof ( request ) )
        return 0;

    return request.base_address;
}
