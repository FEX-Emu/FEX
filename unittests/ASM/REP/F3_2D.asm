%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1",
    "RBX": "0xFFFFFFFFFFFFFFFF",
    "RCX": "0xFFFFFFFE",
    "RDX": "0xFFFFFFFFFFFFFFFC"
  }
}
%endif

mov r15, 0xe0000000

mov rax, 0x414243443f800000
mov [r15 + 8 * 0], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 1], rax

mov rax, 0x41424344bf800000
mov [r15 + 8 * 2], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 3], rax

mov rax, 0x41424344C0000000
mov [r15 + 8 * 4], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 5], rax

mov rax, 0x41424344C0800000
mov [r15 + 8 * 6], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 7], rax

movapd xmm0, [r15 + 8 * 0]
movapd xmm1, [r15 + 8 * 2]

mov rax, -1
mov rbx, -1
mov rcx, -1
mov rdx, -1

cvtss2si eax, xmm0
cvtss2si rbx, xmm1

cvtss2si ecx, [r15 + 8 * 4]
cvtss2si rdx, [r15 + 8 * 6]

hlt
