%ifdef CONFIG
{
  "RegData": {
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
mov ax, 2
mov [rdx + 8 * 1], ax

fld qword [rdx + 8 * 0]
fisubr word [rdx + 8 * 1]
hlt
