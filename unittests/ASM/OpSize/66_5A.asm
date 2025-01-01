%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x404000003F800000", "0x0"],
    "XMM1": ["0x3FF0000000000000", "0x4008000000000000"],
    "XMM2": ["0xff8000007f800000", "0x0000000000000000"],
    "XMM3": ["0x7f8000007fc00000", "0x0000000000000000"]
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3FF0000000000000
mov [rdx + 8 * 0], rax
mov rax, 0x4008000000000000
mov [rdx + 8 * 1], rax

mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 2], rax
mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 3], rax

mov rax, 0x7ff0000000000000
mov [rdx + 8 * 4], rax
mov rax, 0xfff0000000000000
mov [rdx + 8 * 5], rax
mov rax, 0x7ff8000000000000
mov [rdx + 8 * 6], rax
mov rax, 0x7fefffffffffffff
mov [rdx + 8 * 7], rax

movapd xmm0, [rdx + 8 * 2]
movapd xmm1, [rdx]
movapd xmm2, [rdx + 8 * 4]
movapd xmm3, [rdx + 8 * 6]

cvtpd2ps xmm0, xmm1
cvtpd2ps xmm2, xmm2
cvtpd2ps xmm3, xmm3

hlt
