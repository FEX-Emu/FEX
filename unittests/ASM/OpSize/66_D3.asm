%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x20A121A222A323A4", "0x28A929AA2AAB2BAC"],
    "XMM1": ["0x0041424344454647", "0x0051525354555657"],
    "XMM2": ["0x0000414243444546", "0x0000515253545556"],
    "XMM3": ["0x0000000041424344", "0x0000000051525354"],
    "XMM4": ["0x0", "0x0"]
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

mov rax, 0x1
mov [rdx + 8 * 2], rax
mov rax, 0x0
mov [rdx + 8 * 3], rax

mov rax, 0x8
mov [rdx + 8 * 4], rax
mov rax, 0x0
mov [rdx + 8 * 5], rax

mov rax, 0x10
mov [rdx + 8 * 6], rax
mov rax, 0x0
mov [rdx + 8 * 7], rax

mov rax, 0x20
mov [rdx + 8 * 8], rax
mov rax, 0x0
mov [rdx + 8 * 9], rax

; Will Zero
mov rax, 0x40
mov [rdx + 8 * 10], rax
mov rax, 0x0
mov [rdx + 8 * 11], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 0]
movapd xmm2, [rdx + 8 * 0]
movapd xmm3, [rdx + 8 * 0]
movapd xmm4, [rdx + 8 * 0]

psrlq xmm0, [rdx + 8 * 2]
psrlq xmm1, [rdx + 8 * 4]
psrlq xmm2, [rdx + 8 * 6]
psrlq xmm3, [rdx + 8 * 8]
psrlq xmm4, [rdx + 8 * 10]

hlt
