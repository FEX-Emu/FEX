%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x777B7B777B7B7777", "0x7B77777B77777B7B"],
    "XMM1": ["0x8884848884848888", "0x8488888488888484"],
    "XMM2": ["0x777B7B767B7B7776", "0x7B77777A77777B7A"],
    "XMM3": ["0x888484887B7B7777", "0x7B77777A88888484"]
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

aesenclast xmm0, [rdx + 8 * 0]

aesenclast xmm1, [rdx + 8 * 2]

aesenclast xmm2, [rdx + 8 * 4]

aesenclast xmm3, [rdx + 8 * 6]

hlt
