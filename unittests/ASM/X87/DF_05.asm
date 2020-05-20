%ifdef CONFIG
{
  "RegData": {
    "MM7": ["0x8000000000000000", "0x4009"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 1024
mov [rdx + 8 * 0], rax

fild qword [rdx + 8 * 0]

hlt
