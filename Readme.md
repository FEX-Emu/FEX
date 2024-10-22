[中文](https://github.com/FEX-Emu/FEX/blob/main/docs/Readme_CN.md)
# FEX - Fast x86 emulation frontend
FEX allows you to run x86 and x86-64 binaries on an AArch64 host, similar to qemu-user and box86.
It has native support for a rootfs overlay, so you don't need to chroot, as well as some thunklibs so it can forward things like GL to the host.
FEX presents a Linux 5.0+ interface to the guest, and supports only AArch64 as a host.
FEX is very much work in progress, so expect things to change.


## Quick start guide
### For Ubuntu 22.04, 24.04 and 24.10
Execute the following command in the terminal to install FEX through a PPA.

`curl --silent https://raw.githubusercontent.com/FEX-Emu/FEX/main/Scripts/InstallFEX.py --output /tmp/InstallFEX.py && python3 /tmp/InstallFEX.py && rm /tmp/InstallFEX.py`

This command will walk you through installing FEX through a PPA, and downloading a RootFS for use with FEX.

Ubuntu PPA is updated with our monthly releases.

### For everyone else
Please see [Building FEX](#building-fex).

## Getting Started
FEX has been tested to build and run on ARMv8.0+ hardware.
ARMv7 hardware will not work.
Expected operating system usage is Linux. FEX has been tested with Ubuntu 20.04, 20.10, and 21.04. Also Arch Linux.

On AArch64 hosts the user **MUST** have an x86-64 RootFS [Creating a RootFS](#RootFS-Generation).

### Navigating the Source
See the [Source Outline](docs/SourceOutline.md) for more information.

### Building FEX
Follow the guide on the official FEX-Emu Wiki [here](https://wiki.fex-emu.com/index.php/Development:Setting_up_FEX).

### RootFS generation
AArch64 hosts require a rootfs for running applications.
Follow the guide on the wiki page for seeing how to set up the rootfs from scratch
https://wiki.fex-emu.com/index.php/Development:Setting_up_RootFS

![FEX diagram](docs/Diagram.svg)
