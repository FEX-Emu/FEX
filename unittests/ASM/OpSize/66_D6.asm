%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x4142434445464748", "0x0"],
    "XMM1":  ["0x4142434445464748", "0x0"],
    "XMM2":  ["0x4142434445464748", "0x5152535455565758"]
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
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax

movapd xmm2, [rdx + 8 * 0]

; movq xmm0, xmm2
db 0x66, 0x0f, 0xd6, 11_010_000b
movq [rdx + 8 * 2], xmm2
movapd xmm1, [rdx + 8 * 2]

hlt
