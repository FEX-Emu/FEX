%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFFFFFFFF81",
    "RSP": "0xE0000018"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rsp, 0xe0000020

push -127
mov rdx, 0xe0000020
mov rax, [rdx - 8]

hlt
