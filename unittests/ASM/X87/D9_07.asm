%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x37F"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000
fnstcw [rdx]
mov eax, 0
mov ax, [rdx]

hlt
