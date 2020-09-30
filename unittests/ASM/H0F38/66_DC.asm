%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x77637B6F637B6F77", "0x7B6F77636F77637B"],
    "XMM1": ["0x889C84909C849088", "0x8490889C90889C84"],
    "XMM2": ["0x77637B6E637B6F76", "0x7B6F77626F77637A"],
    "XMM3": ["0x889C8490637B6F77", "0x7B6F776290889C84"]
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

aesenc xmm0, [rdx + 8 * 0]

aesenc xmm1, [rdx + 8 * 2]

aesenc xmm2, [rdx + 8 * 4]

aesenc xmm3, [rdx + 8 * 6]

hlt
