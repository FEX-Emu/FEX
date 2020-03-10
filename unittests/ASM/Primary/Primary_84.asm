%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0600"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rax, 0x4142434445464847
mov rbx, 0x61
test al, bl

mov rax, 0
lahf

hlt
