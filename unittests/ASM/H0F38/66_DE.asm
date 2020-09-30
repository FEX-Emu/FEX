%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x7A1FC5A0A07A1FC5", "0xC5A07A1F1FC5A07A"],
    "XMM1": ["0x85E03A5F5F85E03A", "0x3A5F85E0E03A5F85"],
    "XMM2": ["0x7A1FC5A1A07A1FC4", "0xC5A07A1E1FC5A07B"],
    "XMM3": ["0x85E03A5FA07A1FC5", "0xC5A07A1EE03A5F85"]
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

mov rax, 0x0202020202020202
mov [rdx + 8 * 8], rax
mov rax, 0x0303030303030303
mov [rdx + 8 * 9], rax

movaps xmm0, [rdx + 8 * 8]
movaps xmm1, [rdx + 8 * 8]
movaps xmm2, [rdx + 8 * 8]
movaps xmm3, [rdx + 8 * 8]

aesdec xmm0, [rdx + 8 * 0]

aesdec xmm1, [rdx + 8 * 2]

aesdec xmm2, [rdx + 8 * 4]

aesdec xmm3, [rdx + 8 * 6]

hlt
