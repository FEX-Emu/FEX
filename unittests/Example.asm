; If you want a specific configuration at the top of asm file then make sure to wrap it in ifdef and endif.
; This allows the python script to extract the json and nasm to ignore the section
;
; X86 State option that can be compared
; - All: Makes sure all options are compared
; - None: No options
; ===== Specific options ====
; -- GPRs --
; RAX, RBX, RCX, RDX
; RSI, RDI, RBP, RSP
; R8-R15
; -- XMM --
; XMM0-XX15
; -- Misc --
; RIP
; FS, GS
; Flags
; -- X87 / MMX / 3DNow --
; MM0-MM7
; ===========================
; Match: Forces full matching of types
;   - Type: String or List of strings
;   - Default: All
; Ignore: Forces types to be ignored when matching. Overwrites Matches
;   - Default: None
;   - Type: String or List of strings
; RegData: Makes sure that a register contains specific data
;   - Default: Any data
;   - Type: Dict of key:value pairs
;   - >64bit registers should contain a list of values for each 64bit value
;
; Additional config options
; ABI : {SystemV-64, Win64, None}
;   - Default: SystemV-64
; StackSize : Stack size that the test needs
;   - Default : 4096
;   - Stack address starts at: [0xc000'0000, 0xc000'0000 + StackSize)
; EntryPoint : Entrypoint for the assembly
;   - Default: 1
;   - 0 is invalid since that is special cased
; MemoryRegions: Memory Regions for the tests to use
;   - Default: No memory regions generated
;   - Dict of key:value pairs
;   - Key indicates the memory base
;   - Value indicates the memory region size
;   - WARNING: Emulator sets up some default regions that you don't want to intersect with
;   - Additionally the VM only has 64GB of virtual memory. If you go past this size, expect failure
;   - 0xb000'0000 - FS Memory base
;   - 0xc000'0000 - Stack pointer base
;   - 0xd000'0000 - Linux BRK memory base
; MemoryData: Prepopulate one or more memory regions with data
;   - Default: None
;   - Dict of key:value pairs
;   - Key is address
;   - Value is a string with hex data.
;       - No leading 0x needed.
;       - Spaces allowed

%ifdef CONFIG
{
  "Match": "All",
  "Ignore": ["XMM0", "Flags"],
  "RegData": {
    "RAX": "1"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  },
  "MemoryData": {
    "0x100000000" : "00000001 00000000 00000000 00000000",
    "0x100000020" : "fa aa 55 33",
    "0x100000038" : "0x123456789"
  }
}
%endif

mov eax, 1
ret
