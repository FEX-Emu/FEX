%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1",
    "RBX": "0x2",
    "RCX": "0x3",
    "RDX": "0x4",
    "RBP": "0xFFFFFFFE",
    "RSI": "0xFFFFFFFFFFFFFFFC"
  }
}
%endif

mov r15, 0xe0000000

mov rax, 0x414243443f800000
mov [r15 + 8 * 0], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 1], rax

mov rax, 0x4142434440000000
mov [r15 + 8 * 2], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 3], rax

mov rax, 0x4142434440400000
mov [r15 + 8 * 4], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 5], rax

mov rax, 0x4142434440800000
mov [r15 + 8 * 6], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 7], rax

mov rax, 0x41424344C0000000
mov [r15 + 8 * 8], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 9], rax

mov rax, 0x41424344C0800000
mov [r15 + 8 * 10], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 11], rax

movapd xmm0, [r15 + 8 * 0]
movapd xmm1, [r15 + 8 * 2]

mov rax, -1
mov rbx, -1
mov rcx, -1
mov rdx, -1
mov rbp, -1
mov rsi, -1

cvttss2si eax, xmm0
cvttss2si rbx, xmm1

cvttss2si ebp, [r15 + 8 * 8]
cvttss2si rsi, [r15 + 8 * 10]

cvttss2si ecx, [r15 + 8 * 4]
cvttss2si rdx, [r15 + 8 * 6]

hlt
