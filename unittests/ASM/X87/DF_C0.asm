%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0x8000000000000000", "0x4001"],
    "MM7":  ["0x8000000000000000", "0x3FFF"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3ff0000000000000 ; 1.0
mov [rdx + 8 * 0], rax
mov rax, 0x4000000000000000 ; 2.0
mov [rdx + 8 * 1], rax
mov rax, 0x4010000000000000 ; 4.0
mov [rdx + 8 * 2], rax

fld qword [rdx + 8 * 0]
fld qword [rdx + 8 * 1]

; Undocumented x87 instruction
; Sets the tag register to empty for the stack register
; Then pops the stack
ffreep st0
fld qword [rdx + 8 * 2] ; Overwrites previous value

hlt
