%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x0", "0x0"],
    "XMM1": ["0x0000000100000001", "0x0000000100000001"],
    "XMM2": ["0x0000000100000001", "0x0000000100000001"],
    "XMM3": ["0x0000000100000000", "0x0000000100000001"]
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x0000000000000000
mov [rdx + 8 * 0], rax
mov [rdx + 8 * 1], rax

mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 2], rax
mov [rdx + 8 * 3], rax

mov rax, 0x0000000100000001
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax

mov rax, 0xFFFFFFFF00000000
mov [rdx + 8 * 6], rax
mov rax, 0x00000001FFFFFFFF
mov [rdx + 8 * 7], rax

; Test with full zero
pabsd xmm0, [rdx + 8 * 0]

; Test with full negative
pabsd xmm1, [rdx + 8 * 2]

; Test with full positive
pabsd xmm2, [rdx + 8 * 4]

; Test a mix
pabsd xmm3, [rdx + 8 * 6]

hlt
