%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0",
    "RBX": "0x0",
    "RCX": "0xFFFF",
    "XMM0": ["0x4142434445464748", "0x5152535455565758"],
    "XMM1": ["0x0", "0x0"],
    "XMM2": ["0xFFFFFFFFFFFFFFFF", "0xFFFFFFFFFFFFFFFF"]
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

mov rax, 0
mov [rdx + 8 * 2], rax
mov [rdx + 8 * 3], rax
mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 2]
movapd xmm2, [rdx + 8 * 4]

pmovmskb eax, xmm0
pmovmskb ebx, xmm1
pmovmskb ecx, xmm2

hlt
