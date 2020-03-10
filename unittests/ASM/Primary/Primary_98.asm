%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFFFFFFFFF0"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov al, 0xF0
cbw
cwde
cdqe

hlt
