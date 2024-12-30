%ifdef CONFIG
{
  "RegData": {
    "MM0":  "0x0000000200000001",
    "MM1":  "0xFFFFFFFEFFFFFFFF",
    "MM2":  "0x8000000080000000",
    "MM3":  "0x8000000080000000"
  }
}
%endif

mov rdx, 0xe0000000

; Set up MXCSR to truncate
mov eax, 0x7F80
mov [rdx + 8 * 0], eax
ldmxcsr [rdx + 8 * 0]

mov rax, 0x3ff0000000000000
mov [rdx + 8 * 0], rax
mov rax, 0x4000000000000000
mov [rdx + 8 * 1], rax

mov rax, 0xbff0000000000000
mov [rdx + 8 * 2], rax
mov rax, 0xc000000000000000
mov [rdx + 8 * 3], rax

mov rax, 0x4142434445464748
mov [rdx + 8 * 4], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 5], rax

mov rax, 0x7ff0000000000000
mov [rdx + 8 * 6], rax
mov rax, 0xfff0000000000000
mov [rdx + 8 * 7], rax

mov rax, 0x7ff8000000000000
mov [rdx + 8 * 8], rax
mov rax, 0x7fefffffffffffff
mov [rdx + 8 * 9], rax

movq mm0, [rdx + 8 * 4]
movq mm1, [rdx + 8 * 4]

movapd xmm2, [rdx + 8 * 0]

cvtpd2pi mm0, xmm2
cvtpd2pi mm1, [rdx + 8 * 2]
cvtpd2pi mm2, [rdx + 8 * 6]
cvtpd2pi mm3, [rdx + 8 * 8]

hlt
