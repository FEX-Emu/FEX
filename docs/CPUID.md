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
* EBX - **Reserved** - Read as zero
* ECX - **Reserved** - Read as zero
* EDX - **Reserved** - Read as zero

### Sub-Leaf 0000_0001 - FFFF_FFFF: **Reserved**

## 4000_0002h - 4000_000Fh
* **Reserved range**
* Returns zero until implemented

## 4000_0010h - 4FFF_FFFFh
* **Undefined**
* FEX-Emu will return zero until implemented
