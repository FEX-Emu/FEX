# FEX - Virtual Machine Custom CPUBackend
---
This is a custom CPU backend that is used by the FEX frontend for its Lockstep Runner.
This is used for hardware validation of CPU emulation of the FEXCore's emulation.

## Implementation details
This is implemented using the VM helper library that is living inside of [SonicUtils](https://github.com/Sonicadvance1/SonicUtils).
The helper library sets up a VM using KVM under Linux. 

## Limitations
* Only works under KVM
  * I don't care about running under Windows, MacOS, or other hypervisors
* Only works under AMD
  * Haven't spent time figuring out why it breaks, Intel throws a VMX invalid state entry error.
* Can't launch a bunch of instances due to memory limitations
  * Each VM allocates the full VM's memory region upon initialization
  * If you launch a VM with 64GB of memory then that full amount is allocated right away
