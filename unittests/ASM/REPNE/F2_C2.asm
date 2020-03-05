%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xFFFFFFFFFFFFFFFF", "0x4000000000000000"],
    "XMM1": ["0x0", "0x4000000000000000"],
    "XMM2": ["0xFFFFFFFFFFFFFFFF", "0x4000000000000000"],
    "XMM3": ["0x3ff0000000000000", "0x4000000000000000"],
    "XMM4": ["0x0", "0x4000000000000000"],
    "XMM5": ["0xFFFFFFFFFFFFFFFF", "0x4000000000000000"],
    "XMM6": ["0x0", "0x4000000000000000"],
    "XMM7": ["0x3ff0000000000000", "0x4000000000000000"],
    "XMM8": ["0x3ff0000000000000", "0x4008000000000000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3ff0000000000000
mov [rdx + 8 * 0], rax
mov rax, 0x4000000000000000
mov [rdx + 8 * 1], rax

mov rax, 0x3ff0000000000000
mov [rdx + 8 * 2], rax
mov rax, 0x4008000000000000
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 0]
movapd xmm2, [rdx + 8 * 0]
movapd xmm3, [rdx + 8 * 0]
movapd xmm4, [rdx + 8 * 0]
movapd xmm5, [rdx + 8 * 0]
movapd xmm6, [rdx + 8 * 0]
movapd xmm7, [rdx + 8 * 0]
movapd xmm8, [rdx + 8 * 2]

cmpsd xmm0, xmm8, 0x00 ; EQ
cmpsd xmm1, xmm8, 0x01 ; LT
cmpsd xmm2, xmm8, 0x02 ; LTE
;cmpsd xmm3, xmm8, 0x03 ; Unordered
cmpsd xmm4, xmm8, 0x04 ; NEQ
cmpsd xmm5, xmm8, 0x05 ; NLT
cmpsd xmm6, xmm8, 0x06 ; NLTE
;cmpsd xmm7, xmm8, 0x07 ; Ordered

hlt
