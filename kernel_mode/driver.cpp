#include "headers.h"

NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(Irp);
	PVOID pSystemBuffer = Irp->AssociatedIrp.SystemBuffer;

	ULONG cbInputBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbOutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG IoControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;

	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	ULONG cbBytesReturned = 0;

	switch (IoControlCode)
	{
	case IOCTL_ADD:
	{
		if (cbInputBufferLength >= sizeof(int) && cbOutputBufferLength >= sizeof(int) && pSystemBuffer != NULL)
		{
			int* pValue = (int*)pSystemBuffer;
			*pValue = *pValue + 1;
			cbBytesReturned = sizeof(int);
			status = STATUS_SUCCESS;
			DbgPrint("[+] IOCTL_ADD: %d -> %d\n", *pValue - 1, *pValue);
		}
		else
		{
			status = STATUS_BUFFER_TOO_SMALL;
		}
		break;
	}

	case IOCTL_READ:
	{
		if (cbInputBufferLength >= sizeof(KERNEL_READ_REQUEST) && cbOutputBufferLength >= sizeof(KERNEL_READ_REQUEST) && pSystemBuffer != NULL)
		{
			PKERNEL_READ_REQUEST ReadRequest = (PKERNEL_READ_REQUEST)pSystemBuffer;
			PEPROCESS Process = NULL;

			status = PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)ReadRequest->ProcessId, &Process);
			if (!NT_SUCCESS(status))
			{
				DbgPrint("[+] READ: PID %lu not found 0x%08X\n", ReadRequest->ProcessId, status);
				break;
			}

			status = ReadProcessMemory(Process,
				(PVOID)ReadRequest->Address,
				(PVOID)&ReadRequest->Response,
				ReadRequest->Size);

			ObfDereferenceObject(Process);

			if (NT_SUCCESS(status))
			{
				cbBytesReturned = sizeof(KERNEL_READ_REQUEST);
				DbgPrint("[+] READ: PID %lu addr %p -> %llu bytes\n", ReadRequest->ProcessId, (void*)ReadRequest->Address, (unsigned long long)ReadRequest->Size);
			}
			else
			{
				DbgPrint("[+] READ: MmCopyVirtualMemory failed 0x%08X\n", status);
			}
		}
		else
			status = STATUS_BUFFER_TOO_SMALL;
		break;
	}

	case IOCTL_WRITE:
	{
		if (cbInputBufferLength >= sizeof(KERNEL_WRITE_REQUEST) && cbOutputBufferLength >= sizeof(KERNEL_WRITE_REQUEST) && pSystemBuffer != NULL)
		{
			PKERNEL_WRITE_REQUEST WriteRequest = (PKERNEL_WRITE_REQUEST)pSystemBuffer;
			PEPROCESS Process = NULL;

			status = PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)WriteRequest->ProcessId, &Process);
			if (!NT_SUCCESS(status))
			{
				DbgPrint("[+] WRITE: PID %lu not found 0x%08X\n", WriteRequest->ProcessId, status);
				break;
			}

			status = WriteProcessMemory(Process,
				(PVOID)&WriteRequest->Value,
				(PVOID)WriteRequest->Address,
				WriteRequest->Size);

			ObfDereferenceObject(Process);

			if (NT_SUCCESS(status))
			{
				cbBytesReturned = sizeof(KERNEL_WRITE_REQUEST);
				DbgPrint("[+] WRITE: PID %lu addr %p %llu bytes\n", WriteRequest->ProcessId, (void*)WriteRequest->Address, (unsigned long long)WriteRequest->Size);
			}
			else
			{
				DbgPrint("[+] WRITE: failed 0x%08X (demo uses fake addr, may be read-only)\n", status);
			}
		}
		else
			status = STATUS_BUFFER_TOO_SMALL;
		break;
	}

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = cbBytesReturned;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

VOID UnloadDriver(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING sym;
	RtlInitUnicodeString(&sym, SYMLINK_NAME);
	IoDeleteSymbolicLink(&sym);

	if (DriverObject->DeviceObject != NULL)
	{
		IoDeleteDevice(DriverObject->DeviceObject);
	}

	DbgPrint("[+] UnloadDriver: device removed\n");
}

NTSTATUS DriverInitialize(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
	DriverObject->DriverUnload = UnloadDriver;

	UNICODE_STRING dev, sym;
	PDEVICE_OBJECT pDevice = NULL;

	RtlInitUnicodeString(&dev, DEVICE_NAME);

	NTSTATUS status = IoCreateDevice(DriverObject, 0, &dev,
		FILE_DEVICE_UNKNOWN, 0, TRUE, &pDevice);

	if (!NT_SUCCESS(status))
		return status;

	RtlInitUnicodeString(&sym, SYMLINK_NAME);
	status = IoCreateSymbolicLink(&sym, &dev);

	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevice);
		return status;
	}

	pDevice->Flags |= DO_BUFFERED_IO;
	pDevice->Flags &= ~DO_DEVICE_INITIALIZING;

	DbgPrint("[+] Driver loaded\n");

	return STATUS_SUCCESS;
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	if (!DriverObject)
	{
		UNICODE_STRING driverName;
		RtlInitUnicodeString(&driverName, L"\\Driver\\SimpleDriver");
		return IoCreateDriver(&driverName, &DriverInitialize);
	}
	return DriverInitialize(DriverObject, RegistryPath);
}
