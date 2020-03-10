%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

; Set CF to known value
clc

cmc

; Get CF
sbb rax, rax
and rax, 1

hlt
