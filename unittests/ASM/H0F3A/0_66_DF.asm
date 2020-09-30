%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x6363636363636363", "0x6363636363636363"],
    "XMM1": ["0x1616161616161616", "0x1616161616161616"],
    "XMM2": ["0x7c6363636363637c", "0x7c6363636363637c"],
    "XMM3": ["0x1616161616161616", "0x7c6363636363637c"],
    "XMM4": ["0x6363636263636363", "0x6363636263636363"],
    "XMM5": ["0x1616161416161616", "0x1616161416161616"],
    "XMM6": ["0x7c6363606363637c", "0x7c6363606363637c"],
    "XMM7": ["0x1616161216161616", "0x7c6363676363637c"],
    "XMM8": ["0x6363636663636363", "0x6363636663636363"],
    "XMM9": ["0x1616161016161616", "0x1616161016161616"],
    "XMM10": ["0x7c6363646363637c", "0x7c6363646363637c"],
    "XMM11": ["0x1616161e16161616", "0x7c63636b6363637c"],
    "XMM12": ["0x6363636a63636363", "0x6363636a63636363"],
    "XMM13": ["0x1616161c16161616", "0x1616161c16161616"],
    "XMM14": ["0x7c6363686363637c", "0x7c6363686363637c"],
    "XMM15": ["0x1616161a16161616", "0x7c63636f6363637c"]
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

aeskeygenassist xmm0, [rdx + 8 * 0], 0
aeskeygenassist xmm1, [rdx + 8 * 2], 0
aeskeygenassist xmm2, [rdx + 8 * 4], 0
aeskeygenassist xmm3, [rdx + 8 * 6], 0

aeskeygenassist xmm4, [rdx + 8 * 0], 1
aeskeygenassist xmm5, [rdx + 8 * 2], 2
aeskeygenassist xmm6, [rdx + 8 * 4], 3
aeskeygenassist xmm7, [rdx + 8 * 6], 4

aeskeygenassist xmm8, [rdx + 8 * 0], 5
aeskeygenassist xmm9, [rdx + 8 * 2], 6
aeskeygenassist xmm10, [rdx + 8 * 4], 7
aeskeygenassist xmm11, [rdx + 8 * 6], 8

aeskeygenassist xmm12, [rdx + 8 * 0], 9
aeskeygenassist xmm13, [rdx + 8 * 2], 10
aeskeygenassist xmm14, [rdx + 8 * 4], 11
aeskeygenassist xmm15, [rdx + 8 * 6], 12

hlt
