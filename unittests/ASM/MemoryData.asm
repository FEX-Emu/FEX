%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xddccbbaa"
  },
  "MemoryRegions": {
    "0x100000": "4096"
  },
  "MemoryData": {
    "0x100000": "AA BB CC DD"
  }
}
%endif

; Simple test to prove that config loader's MemoryData is working

mov rax, [0x100000]
hlt
