BITS 64
%ifdef CONFIG
{
  "RegData": {
    "RSP": "0x100000002", "RBP": "0x100007788"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  },
  "MemoryData": {
    "0x100000000": "88 77 66 55 44 33 22 11"
  }
}
%endif


mov rsp, 0xdeadbeef
mov rbp, 0x100000000

o16 leave

hlt