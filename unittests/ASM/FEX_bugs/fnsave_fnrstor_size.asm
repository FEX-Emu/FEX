%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x4142434445464748",
    "RCX": "0x5152535455565758"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

; FEX-Emu implements an optimization for fnsave and frstor of overlapping 16-byte read and writes for the x87 registers.
; This test ensures that these instructions don't exceed the storage limits imposed by the instruction details.
; Ensuring that changes like from https://github.com/FEX-Emu/FEX/pull/4107 would get picked up by unit tests.

; Calculate address to the end of the memory region.
mov rax, 0x1_0000_0000 + 4096

; Save at the end of the page to ensure it doesn't fault.
fnsave [rax - 108]

; Do an frstor at the end of the page to ensure it doesn't fault.
frstor [rax - 108]

; Save at the end of the page to ensure it doesn't fault.
o16 fnsave [rax - 94]

; Do an frstor at the end of the page to ensure it doesn't fault.
o16 frstor [rax - 94]

; Store data at the end.
mov rbx, 0x4142434445464748
mov [rax - 8], rbx

; Save just before the end of the data we stored.
; Ensures we don't accidentally overwrite data.
fnsave [rax - 116]

; Load back the register to ensure it still contains the correct data
mov rbx, [rax - 8]

; Store data at the end.
mov rcx, 0x5152535455565758
mov [rax - 8], rcx

; Save just before the end of the data we stored.
; Ensures we don't accidentally overwrite data.
o16 fnsave [rax - 102]

; Load back the register to ensure it still contains the correct data
mov rcx, [rax - 8]

hlt
