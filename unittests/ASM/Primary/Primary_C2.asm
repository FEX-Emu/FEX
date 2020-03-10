%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1",
    "RSP": "0xE000FF20"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rsp, 0xe0000020
lea rax, [rel .end]
push rax

mov rax, 1
ret 0xFF00
mov rax, 0

.end:
hlt
