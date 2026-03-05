#include "headers.h"

int main()
{
	printf("[+] SimpleDriver - Read/Write external process (notepad)\n\n");

	HANDLE hDevice = CreateFileA("\\\\.\\SimpleDriver",
		GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("[!] CreateFile failed (%lu). Driver loaded?\n", GetLastError());
		system("pause");
		return 1;
	}

	ULONG pid = GetPidByName(L"notepad.exe");

	if (pid == 0)
	{
		printf("[!] Run notepad.exe before starting.\n");
		CloseHandle(hDevice);
		system("pause");
		return 1;
	}

	ULONG_PTR base = GetModuleBase(pid, L"notepad.exe");
	if (base == 0)
	{
		printf("[!] Could not get module base.\n");
		CloseHandle(hDevice);
		system("pause");
		return 1;
	}

	printf("[+] Target: notepad.exe PID %lu base %p\n", pid, (void*)base);

	ULONG_PTR valueRead = 0;

	if (ReadMemory(hDevice, pid, base, 2, &valueRead))
	{
		WORD mz = (WORD)valueRead;
		printf("[+] READ: MZ 0x%04X (%c%c)\n", mz, (char)(mz & 0xFF), (char)(mz >> 8));
	}
	else
	{
		printf("[!] READ failed %lu\n", GetLastError());
	}


	// base+0xD000 may be invalid for notepad (read-only); WRITE can fail - demo only
	ULONG_PTR writeAddr = base + 0xD000;
	const int testValue = 666;

	if (WriteMemory(hDevice, pid, writeAddr, testValue, sizeof(int)))
	{
		printf("[+] WRITE: %d -> %p\n", testValue, (void*)writeAddr);
	}
	else
	{
		printf("[!] WRITE failed (addr invalid for notepad, demo only)\n");
	}

	valueRead = 0;

	if (ReadMemory(hDevice, pid, writeAddr, sizeof(int), &valueRead))
	{
		printf("[+] READ verify: %llu\n", (unsigned long long)valueRead);
	}

	CloseHandle(hDevice);
	printf("\n[+] Done\n");
	system("pause");
	return 0;
}
