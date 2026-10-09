# Windows Kernel IOCTL Demo

Communicate with a Windows kernel driver and read/write process memory through IOCTLs

## How does it work?

The user-mode app opens `\\.\SimpleDriver` and sends requests through `DeviceIoControl`. The WDM driver handles them in kernel space using buffered I/O.

- `IOCTL_ADD`: adds 1 to an integer.
- `IOCTL_READ` / `IOCTL_WRITE`: read and write process memory.
- `GET_MODULE`: finds a module base address through the target process PEB/LDR.

The driver is in `kernel_mode/`; the client is in `user_mode/`.

## Demo

![IOCTL demo](demo.png)

The WRITE example uses a hardcoded address and may fail. Set a valid address for your own test process before using it.

Enable Windows test mode before loading the driver with `sc create` / `sc start`. Outside test mode, the driver needs signing or a custom loader.

NOTE: THIS IS FOR EDUCATIONAL PURPOSES ONLY. Test in a VM.

This is part of my Windows internals studies. More notes on [my blog](https://cnthigu.github.io/).
