%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

clc

; Get CF
sbb rax, rax
and rax, 1

hlt
