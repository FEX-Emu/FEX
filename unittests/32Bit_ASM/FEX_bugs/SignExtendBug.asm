%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x41424344"
  },
  "MemoryRegions": {
    "0xf0000000": "4096"
  },
  "MemoryData": {
    "0xf0000000": "0x41424344"
  },
  "Mode": "32BIT"
}
%endif

; Ensures that zero extension of addresses are adhered to.
lea eax, [0xf000_0000]
mov eax, [ds:eax]

hlt
