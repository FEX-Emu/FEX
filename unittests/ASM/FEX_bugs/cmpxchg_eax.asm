%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x00000000ccddff00",
    "RBX": "0x8899aabbccddff00"
  }
}
%endif

mov rax, 0x0011223344556677
mov rbx, 0x8899AABBCCDDFF00

cmpxchg eax, ebx

hlt