%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x0000000000000000", "0x0000000000000000"],
    "XMM1": ["0xffffffffffffffff", "0xffffffffffffffff"],
    "XMM2": ["0x0b0d090e0b0d090e", "0x0b0d090e0b0d090e"],
    "XMM3": ["0xffffffff00000000", "0x0b0d090effffffff"],
    "XMM4": ["0x0202020202020202", "0x0303030303030303"]
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

aesimc xmm0, [rdx + 8 * 0]
aesimc xmm1, [rdx + 8 * 2]
aesimc xmm2, [rdx + 8 * 4]
aesimc xmm3, [rdx + 8 * 6]
aesimc xmm4, [rdx + 8 * 8]

hlt
