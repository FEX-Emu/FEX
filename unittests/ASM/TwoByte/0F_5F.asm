%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x40c0000040a00000", "0x4100000040e00000"],
    "XMM1": ["0x40c0000040a00000", "0x4100000040e00000"],
    "XMM3": ["0xffc000007fc00000", "0x7fc00000ffc00000"],
    "XMM5": ["0x40c0000040a00000", "0x4100000040e00000"],
    "XMM6": ["0x0000000800000000", "0x0000000800000000"]
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x400000003f800000 ; 2, 1
mov [rdx + 8 * 0], rax
mov rax, 0x4080000040400000 ; 4, 3
mov [rdx + 8 * 1], rax

mov rax, 0x40c0000040a00000 ; 6, 5
mov [rdx + 8 * 2], rax
mov rax, 0x4100000040e00000 ; 8, 7
mov [rdx + 8 * 3], rax

mov rax, 0xFFC000007FC00000 ; NaN, NaN
mov [rdx + 8 * 4], rax
mov rax, 0x7FC00000FFC00000 ; NaN, NaN
mov [rdx + 8 * 5], rax

mov rax, 0x8000000000000000 ; -0, 0
mov [rdx + 8 * 6], rax
mov rax, 0x0000000800000000 ; 0, -0
mov [rdx + 8 * 7], rax

mov rax, 0x0000000800000000 ; 0, -0
mov [rdx + 8 * 8], rax
mov rax, 0x8000000000000000 ; -0, 0
mov [rdx + 8 * 9], rax

movapd xmm0, [rdx]
maxps xmm0, [rdx + 8 * 2]

movapd xmm1, [rdx]
movapd xmm2, [rdx + 8 * 2]
maxps xmm1, xmm2

movapd xmm3, [rdx]
movapd xmm4, [rdx + 8 * 2]
movapd xmm5, [rdx + 8 * 4]
movapd xmm6, [rdx + 8 * 6]
movapd xmm7, [rdx + 8 * 8]
maxps xmm3, xmm5 ; NaN on src side
maxps xmm5, xmm4 ; NaN on dst side
maxps xmm6, xmm7 ; 0's on both sides

hlt
