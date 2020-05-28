%ifdef CONFIG
{
  "RegData": {
    "MM7":  ["0x8000000000000000", "0x3FFE"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3ff0000000000000 ; 1.0
mov [rdx + 8 * 0], rax
mov eax, 2
mov [rdx + 8 * 1], eax

fld qword [rdx + 8 * 0]
fidiv dword [rdx + 8 * 1]
hlt
