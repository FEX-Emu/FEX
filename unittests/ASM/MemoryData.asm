%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xddccbbaa"
  },
  "MemoryRegions": {
    "0x10000000": "4096"
  },
  "MemoryData": {
    "0x10000000": "AA BB CC DD"
  }
}
%endif

; Simple test to prove that config loader's MemoryData is working

mov rax, [abs 0x10000000]
hlt
