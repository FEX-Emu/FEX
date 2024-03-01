%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x41424344",
    "RBX": "0x41424344",
    "RCX": "0x51525354"
  },
  "MemoryRegions": {
    "0x00fd0000": "4096",
    "0xf0000000": "4096"
  },
  "MemoryData": {
    "0xf0000000": "0x41424344",
    "0x00fd0000": "0x51525354"
  },
  "Mode": "32BIT"
}
%endif

; Ensures that zero extension of addresses are adhered to.
lea eax, [0xf000_0000]
mov eax, [ds:eax]

; Ensures that zext occurs correctly with two registers that have the sign bit set.
mov ebx, 0xffff_ffff
mov ecx, 0xf000_0001

; Break the block so it can't optimize through.
jmp .test
.test:
mov ebx, [ebx+ecx]

; Ensures that zext occurs correctly with SIB indexing with second argument not having sign bit set but "index" having sign bit.
; Originally saw in Metal Gear Rising Revengeance with a `jmp dword [ecx*4+0xfdbf10]` instruction.
; With ecx = 0xfffffff4 = -12. This is them loading a switch table's branches just before the switch base.
mov ecx, -12

; Break the block so it can't optimize through.
jmp .test2
.test2:

mov ecx, [ecx*4+0x00fd_0030]
hlt
