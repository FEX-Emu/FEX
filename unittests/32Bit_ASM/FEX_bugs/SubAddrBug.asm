%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xdeadbeef"
  },
  "MemoryRegions": {
    "0x10000000": "4096"
  },
  "MemoryData": {
    "0x10000000": "0xdeadbeef"
  },
  "Mode": "32BIT"
}
%endif

section .text

lea eax, [0x10000040]
mov eax, [eax-0x40]
hlt
