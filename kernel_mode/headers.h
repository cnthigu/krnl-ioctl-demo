#pragma once

#include <ntddk.h>
#include <wdm.h>

#define IOCTL_ADD   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define DEVICE_NAME  L"\\Device\\SimpleDriver"
#define SYMLINK_NAME L"\\DosDevices\\SimpleDriver"

extern "C" NTKERNELAPI NTSTATUS IoCreateDriver(
	_In_opt_ PUNICODE_STRING DriverName,
	_In_ PDRIVER_INITIALIZE InitializationFunction);

extern "C" NTKERNELAPI NTSTATUS PsLookupProcessByProcessId(
	_In_ HANDLE ProcessId,
	_Outptr_ PEPROCESS* Process);

extern "C" NTSTATUS NTAPI MmCopyVirtualMemory(
	PEPROCESS SourceProcess,
	PVOID SourceAddress,
	PEPROCESS TargetProcess,
	PVOID TargetAddress,
	SIZE_T BufferSize,
	KPROCESSOR_MODE PreviousMode,
	PSIZE_T ReturnSize);

#pragma pack(push, 1)
typedef struct _KERNEL_READ_REQUEST {
	ULONG ProcessId;
	ULONG_PTR Address;
	ULONG_PTR Response;
	SIZE_T Size;
} KERNEL_READ_REQUEST, * PKERNEL_READ_REQUEST;

typedef struct _KERNEL_WRITE_REQUEST {
	ULONG ProcessId;
	ULONG_PTR Address;
	ULONG_PTR Value;
	SIZE_T Size;
} KERNEL_WRITE_REQUEST, * PKERNEL_WRITE_REQUEST;
#pragma pack(pop)

NTSTATUS ReadProcessMemory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size)
{
	PEPROCESS SourceProcess = Process;
	PEPROCESS TargetProcess = PsGetCurrentProcess();
	SIZE_T BytesWritten = 0;

	NTSTATUS status = MmCopyVirtualMemory(
		SourceProcess, SourceAddress,
		TargetProcess, TargetAddress,
		Size, KernelMode, &BytesWritten);

	return status;
}

NTSTATUS WriteProcessMemory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size)
{
	PEPROCESS SourceProcess = PsGetCurrentProcess();
	PEPROCESS TargetProcess = Process;
	SIZE_T BytesWritten = 0;

	NTSTATUS status = MmCopyVirtualMemory(
		SourceProcess, SourceAddress,
		TargetProcess, TargetAddress,
		Size, KernelMode, &BytesWritten);

	return status;
}
