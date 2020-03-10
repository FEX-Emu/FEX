%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFF81828384",
    "RBX": "0x0000000071727374"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rax, 0x81828384
mov rbx, 0x71727374
movsxd rax, eax
movsxd rbx, ebx

hlt
