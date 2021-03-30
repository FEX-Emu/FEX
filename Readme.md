# FEX - Fast x86 emulation frontend
FEX allows you to run x86 and x86-64 binaries on an AArch64 host, similar to qemu-user and box86.
It has native support for a rootfs overlay, so you don't need to chroot, as well as some thunklibs so it can forward things like GL to the host.
FEX presents a Linux 5.0 interface to the guest, and supports both AArch64 and x86-64 as hosts.
FEX is very much work in progress, so expect things to change.

## Getting Started
FEX has been tested to build and run on ARMv8.0, ARMv8.1+, and x86-64(AVX or newer) hardware.
ARMv7 and older x86 hardware will not work.
Expected operating system usage is Linux. FEX has been tested with Ubuntu 20.04, 20.10, and 21.04. Also Arch Linux.
On AArch64 hosts the user **MUST** have an x86-64 RootFS [Creating a RootFS](#RootFS-Generation).

### Navigating the Source
See the [Source Outline](docs/SourceOutline.md) for more information.

### Dependencies
* cmake (version 3.14 minimum)
* ninja-build
* clang (version 10 minimum for C++20)
* libnuma-dev
* libglfw3-dev (For GUI)
* libsdl2-dev (For GUI)
* libepoxy-dev (For GUI)
* g++-x86-64-linux-gnu (For building thunks)
* nasm (only if building tests)

### Building FEX
After installing the dependencies you can now build FEX.
```Shell
git clone https://github.com/FEX-Emu/FEX.git
cd FEX
git submodule update --init
mkdir Build
cd Build
CC=clang CXX=clang++ cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=True -DBUILD_TESTS=False -G Ninja ..
ninja
```

### Installation
```Shell
sudo ninja install
```

#### On AArch64 Hosts
You can install a binfmt_misc handler for both 32bit and 64bit x86 execution directly from the environment. If you already have box86's 32bit binfmt_misc handler installed then I don't recommend installing FEX's until it is useful. Make sure to have run install prior to this, otherwise binfmt_misc will install an old handler even if the executable has been updated.
```Shell
sudo ninja binfmt_misc_32
sudo ninja binfmt_misc_64
```

### More information
This wiki page can contain more information about setting up FEX on your device
https://wiki.fex-emu.org/index.php/Development:Setting_up_FEX

### RootFS generation
AArch64 hosts require a rootfs for running applications.
Follow the guide on the wiki page for seeing how to set up the rootfs from scratch
https://wiki.fex-emu.org/index.php/Development:Setting_up_RootFS

![FEX diagram](docs/Diagram.svg)
