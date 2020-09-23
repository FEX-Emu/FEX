%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1",
    "RBX": "0x2",
    "RCX": "0x3",
    "RDX": "0x4",
    "RBP": "0xFFFFFFFE",
    "RSI": "0xFFFFFFFFFFFFFFFC"
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

mov rax, 0x4142434440000000
mov [rdx + 8 * 2], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 3], rax

mov rax, 0x4142434440400000
mov [rdx + 8 * 4], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 5], rax

mov rax, 0x4142434440800000
mov [rdx + 8 * 6], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 7], rax

mov rax, 0x41424344C0000000
mov [rdx + 8 * 8], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 9], rax

mov rax, 0x41424344C0800000
mov [rdx + 8 * 10], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 11], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 2]

cvttss2si eax, xmm0
cvttss2si rbx, xmm1

cvttss2si ebp, [rdx + 8 * 8]
cvttss2si rsi, [rdx + 8 * 10]

cvttss2si ecx, [rdx + 8 * 4]
cvttss2si rdx, [rdx + 8 * 6]

hlt
