%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000
wrgsbase rdx

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax

mov rax, -1
mov rax, qword [gs:0]

hlt
