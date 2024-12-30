%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1",
    "RBX": "0x2",
    "RCX": "0x3",
    "RDX": "0x4",
    "R9": "0x8000000000000000",
    "R10": "0x8000000000000000",
    "R11": "0x8000000000000000",
    "R12": "0x8000000000000000"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3FF0000000000000
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x4000000000000000
mov [rdx + 8 * 2], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 3], rax

mov rax, 0x4008000000000000
mov [rdx + 8 * 4], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 5], rax

mov rax, 0x4010000000000000
mov [rdx + 8 * 6], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 7], rax

mov rax, 0x7ff0000000000000
mov [rdx + 8 * 8], rax
mov rax, 0xfff0000000000000
mov [rdx + 8 * 9], rax
mov rax, 0x7ff8000000000000
mov [rdx + 8 * 10], rax
mov rax, 0x7fefffffffffffff
mov [rdx + 8 * 11], rax


movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 2]

cvtsd2si eax, xmm0
cvtsd2si rbx, xmm1

cvtsd2si ecx, [rdx + 8 * 4]
cvtsd2si r9, [rdx + 8 * 8]
cvtsd2si r10, [rdx + 8 * 9]
cvtsd2si r11, [rdx + 8 * 10]
cvtsd2si r12, [rdx + 8 * 11]
cvtsd2si rdx, [rdx + 8 * 6]

hlt
