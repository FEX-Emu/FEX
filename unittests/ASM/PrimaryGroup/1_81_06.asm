%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434424244748",
    "RBX": "0x515253543434343C",
    "RCX": "0x616263640404040C",
    "RDX": "0x9E9D9C9B9A999868"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax
mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax
mov rax, 0x6162636465666768
mov [rdx + 8 * 3], rax

xor  word [rdx + 8 * 0 + 2], 0x6162
xor dword [rdx + 8 * 1 + 0], 0x61626364
xor qword [rdx + 8 * 2 + 0], 0x61626364
xor qword [rdx + 8 * 3 + 0], -256

mov rax, [rdx + 8 * 0]
mov rbx, [rdx + 8 * 1]
mov rcx, [rdx + 8 * 2]
mov rdx, [rdx + 8 * 3]

hlt
