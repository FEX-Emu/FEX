%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RBX": "0x5152535455565758",
    "RCX": "0x5152535455565758"
  },

  "MemoryRegions": {
    "0x7FFFF000": "4096"
  },

  "MemoryData": {
    "0x7FFFF000": "48 47 46 45 44 43 42 41"
  }
}
%endif

; Tests to ensure that 64-bit displacement encoding works correctly without being RIP relative.
; x86-64 has two displacement encodings, one is RIP relative, one is 32-bit (signed) displacement only.
; modrm.mod = 0b00 && modrm.rm = 0b101: Means no SIB, but address mode is RIP + disp32.
; modrm.mod = 0b00 && modrm.rm = 0b100: Means SIB, but address mode is disp32.
;  - if SIB.base = 0b101 && SIB.index = 0b100. Which means no registers for base and index.
; Test disp32 by mapping a page at the limit of 2GB and read data from it. Also store and load.
; If we were accidentally using RIP relative, then it would be 2GB + <low test base address>, which won't be mapped.

; Test disp32 load.
mov rax, [0x7FFF_F000]

mov rbx, 0x5152535455565758

; LEA with disp32.
lea rcx, [0x7FFF_FFF8]

; Test store with disp32 store.
mov [0x7FFF_FFF8], rbx

; Load back with the LEA to ensure it's correct.
mov rcx, [rcx]

hlt
