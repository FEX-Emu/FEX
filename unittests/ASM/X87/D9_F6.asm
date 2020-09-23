%ifdef CONFIG
{
  "RegData": {
    "MM4":  ["0x8000000000000000", "0x4001"],
    "MM5":  ["0x0000000000000000", "0x0000"],
    "MM6":  ["0x8000000000000000", "0x4000"],
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

; Do Some stack shenanigans
fldz
fincstp
fdecstp

fld qword [rdx + 8 * 2]

hlt
