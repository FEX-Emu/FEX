%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RSP": "0xE0000FD8",
    "RBP": "0xE0000FF8"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rsp, 0xe0001000
mov rbp, 0xe0002000
mov rax, 0x4142434445464748
mov qword [rbp - 8], rax

enter 0x10, 2
mov rax, qword [rsp + 0x18]

hlt

