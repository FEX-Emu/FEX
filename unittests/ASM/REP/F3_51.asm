%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x414243443f800000", "0x5152535455565758"],
    "XMM1":  ["0x4142434440000000", "0x5152535455565758"],
    "XMM2":  ["0x4142434440400000", "0x5152535455565758"],
    "XMM3":  ["0x4142434440800000", "0x5152535455565758"],
    "XMM4":  ["0x414243443f800000", "0x5152535455565758"],
    "XMM5":  ["0x4142434440000000", "0x5152535455565758"],
    "XMM6":  ["0x4142434440400000", "0x5152535455565758"],
    "XMM7":  ["0x4142434440800000", "0x5152535455565758"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x414243443f800000 ; 1.0
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x4142434440800000 ; 4.0
mov [rdx + 8 * 2], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 3], rax

mov rax, 0x4142434441100000 ; 9.0
mov [rdx + 8 * 4], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 5], rax

mov rax, 0x4142434441800000 ; 16.0
mov [rdx + 8 * 6], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 7], rax

mov rax, 0x4142434441c80000 ; 25.0
mov [rdx + 8 * 8], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 9], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 2]
movapd xmm2, [rdx + 8 * 4]
movapd xmm3, [rdx + 8 * 6]
movapd xmm4, [rdx + 8 * 8]
movapd xmm5, [rdx + 8 * 8]
movapd xmm6, [rdx + 8 * 8]
movapd xmm7, [rdx + 8 * 8]

sqrtss xmm0, xmm0
sqrtss xmm1, xmm1
sqrtss xmm2, xmm2
sqrtss xmm3, xmm3

sqrtss xmm4, [rdx + 8 * 0]
sqrtss xmm5, [rdx + 8 * 2]
sqrtss xmm6, [rdx + 8 * 4]
sqrtss xmm7, [rdx + 8 * 6]

hlt
