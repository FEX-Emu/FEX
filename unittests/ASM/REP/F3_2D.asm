%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1",
    "RBX": "0xFFFFFFFFFFFFFFFF",
    "RCX": "0xFFFFFFFE",
    "RDX": "0xFFFFFFFFFFFFFFFC"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x414243443f800000
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x41424344bf800000
mov [rdx + 8 * 2], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 3], rax

mov rax, 0x41424344C0000000
mov [rdx + 8 * 4], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 5], rax

mov rax, 0x41424344C0800000
mov [rdx + 8 * 6], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 7], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 2]

cvtss2si eax, xmm0
cvtss2si rbx, xmm1

cvtss2si ecx, [rdx + 8 * 4]
cvtss2si rdx, [rdx + 8 * 6]

hlt
