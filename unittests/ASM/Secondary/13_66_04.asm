%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x0000000000000000", "0x0000000000000000"],
    "XMM1": ["0x0000616200006566", "0x0000717200007576"],
    "XMM2": ["0x0041424300454647", "0x0051525300555657"],
    "XMM3": ["0x30B131B232B333B4", "0x38B939BA3ABB3BBC"],
    "XMM4": ["0xFFFFFFFFFFFFFFFF", "0x0000000000000000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 3], rax

mov rax, 0x8000800080008000
mov [rdx + 8 * 4], rax
mov rax, 0x7000700070007000
mov [rdx + 8 * 5], rax

movapd xmm0, [rdx]
movapd xmm1, [rdx + 16]
movapd xmm2, [rdx]
movapd xmm3, [rdx + 16]
movapd xmm4, [rdx + 32]

psrad xmm0, 32
psrad xmm1, 16
psrad xmm2, 8
psrad xmm3, 1
psrad xmm4, 32

hlt
