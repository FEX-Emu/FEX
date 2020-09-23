%ifdef CONFIG
{
  "RegData": {
    "MM0":  ["0x8000000000000000", "0x4007"],
    "MM1":  ["0x8000000000000000", "0x4006"],
    "MM2":  ["0x8000000000000000", "0x4005"],
    "MM3":  ["0x8000000000000000", "0x4004"],
    "MM4":  ["0x8000000000000000", "0x4003"],
    "MM5":  ["0x8000000000000000", "0x4002"],
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
mov rax, 0x4020000000000000 ; 4.0
mov [rdx + 8 * 2], rax
mov rax, 0x4030000000000000
mov [rdx + 8 * 3], rax
mov rax, 0x4040000000000000
mov [rdx + 8 * 4], rax
mov rax, 0x4050000000000000
mov [rdx + 8 * 5], rax
mov rax, 0x4060000000000000
mov [rdx + 8 * 6], rax
mov rax, 0x4070000000000000
mov [rdx + 8 * 7], rax

fld qword [rdx + 8 * 0]
fld qword [rdx + 8 * 1]
fld qword [rdx + 8 * 2]
fld qword [rdx + 8 * 3]
fld qword [rdx + 8 * 4]
fld qword [rdx + 8 * 5]
fld qword [rdx + 8 * 6]
fld qword [rdx + 8 * 7]
fincstp

hlt
