%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0x8000000000000000", "0x3FFE"],
    "MM7":  ["0x8000000000000000", "0x4000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4000000000000000 ; 2.0
mov [rdx + 8 * 0], rax
mov rax, 0x4010000000000000 ; 4.0
mov [rdx + 8 * 1], rax

fld qword [rdx + 8 * 0]
fld qword [rdx + 8 * 1]

fdivr st0, st1
hlt
