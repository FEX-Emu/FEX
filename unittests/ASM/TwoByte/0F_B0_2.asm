%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000055565758",
    "RBX": "0x5152535455565758",
    "RCX": "0x6162636465666768"
  }
}
%endif

mov r10, 0xe0000000

mov rax, 0x4142434445464748
mov rbx, 0x5152535455565758
mov rcx, 0x6162636465666768
cmpxchg ebx, ecx

hlt
