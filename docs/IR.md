# FEXCore IR
---
The IR for the FEXCore is an SSA based IR that is generated from the incoming x86-64 assembly.
SSA is quite nice to work with when translating the x86-64 code to the IR, when optimizing that code with custom optimization passes, and also passing that IR to our CPU backends.

## Emulation IR considerations
* We have explicitly sized IR variables
  * Supports traditional element sizes of 1,2,4,8 bytes and some 16byte ops
  * Supports arbitrary number of vector elements
  * The op determines if something is float or integer based.
* Clear separation of scalar IR ops and vector IR ops
  * ex, MUL versus VMUL
* We have explicit Load/Store context IR ops
  * This allows us to have a clear separation between guest memory and tracked x86-64 state
* We have an explicit CPUID IR op
  * This allows us to return fairly complex data (4 registers of data) and also having an easier optimization for constant CPUID functions
  * So if we const-prop the CPUID function then it'll just const-prop further along
* We have an explicit syscall op
  * The syscall op is fairly complex as well, same with CPUID that if the syscall function is const-prop then we can directly call the syscall handler
  * Can save overhead by removing call overheads
* The IR supports branching from one block to another
  * Has a conditional branch instruction that either branches to the target branch or falls through to the next block
  * Has an unconditional branch to explicitly jump to a block instead of falling through
  * **There is a desire to follow LLVM semantics around block limitations but it isn't currently strictly enforced**
* Supports a debug ```Print``` Op for printing out values for debug viewing
* Supports explicit Load/Store memory IR ops
  * This is for accessing guest memory and will do the memory offset translation in to the VM's memory space
  * This is done by just adding the VM memory base to the 64bit address passed in
  * This is done in a manner that the application **can** escape from the VM and isn't meant to be safe
  * There is an option for JITs to validate the memory region prior to accessing for ensuring correctness
* IR is generated from a JSON file, fairly straightforward to extend.
  * Read the python generation file to determine the extent of what it can do
