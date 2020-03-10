%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFFBEBDBCBB",
    "RBX": "0x51526162",
    "RSP": "0xE0000014"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rsp, 0xe0000020

push qword -0x41424345
push word 0x5152
push word 0x6162

mov rdx, 0xe0000020
mov rax, [rdx - 8]
mov ebx, [rdx - 12]

hlt
