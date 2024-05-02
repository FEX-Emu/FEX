# FEXCore custom CPUID functions

## 4000_0000h - Hypervisor information function
* Follows VMWare and Microsoft's hypervisor information proposal
* https://lwn.net/Articles/301888/
* https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/feature-discovery

* EAX - The maximum input value for the hypervisor CPUID information
  * 4000_0001h
* EBX - Hypervisor vendor ID signature
  * 'FEXI' - 4958_4546h
* ECX - Hypervisor vendor ID signature
  * 'FEXI' - 4958_4546h
* EDX - Hypervisor vendor ID signature
  * 'EMU\0' - 0055_4d45h

* memcpy ebx:ecx:edx in to a 12 byte string to get 'FEXIFEXIEMU\0' for determining running under FEX

## 4000_0001h - Hypervisor config function

### Sub-Leaf 0: ECX == 0
* EAX:
  * Bits EAX[3:0] - Host architecture
    * 0 - Unknown architecture
    * 1 - x86_64
    * 2 - AArch64
    * 3-15: **Reserved**
  * Bits EAX[15:4] - **Reserved**
  * Bits EAX[31:16] - Maximum subleaf input value for CPUID function 4000_0001h
* EBX - **Reserved** - Read as zero
* ECX - **Reserved** - Read as zero
* EDX - **Reserved** - Read as zero

### Sub-leaf 1: ECX == 1
* FEX version string signature. First 16-bytes
* memcpy eax:ebx:ecx:edx in to the first 16-bytes of a string.

### Sub-leaf 2: ECX == 2
* FEX version string signature. Second 16-bytes
* memcpy eax:ebx:ecx:edx in to the second 16-bytes of a string.

### Sub-Leaf 0000_0003 - FFFF_FFFF: **Reserved**

## 4000_0002h - 4000_000Fh
* **Reserved range**
* Returns zero until implemented

## 4000_0010h - 4FFF_FFFFh
* **Undefined**
* FEX-Emu will return zero until implemented
