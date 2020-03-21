%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x3ff0000000000000", "0x3ff0000000000000"],
    "XMM1":  ["0x4000000000000000", "0x4000000000000000"],
    "XMM2":  ["0x4008000000000000", "0x4008000000000000"],
    "XMM3":  ["0x4010000000000000", "0x4010000000000000"],
    "XMM4":  ["0x3ff0000000000000", "0x3ff0000000000000"],
    "XMM5":  ["0x4000000000000000", "0x4000000000000000"],
    "XMM6":  ["0x4008000000000000", "0x4008000000000000"],
    "XMM7":  ["0x4010000000000000", "0x4010000000000000"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3FF0000000000000 ; 1.0
mov [rdx + 8 * 0], rax
mov rax, 0x3FF0000000000000
mov [rdx + 8 * 1], rax

mov rax, 0x4010000000000000 ; 4.0
mov [rdx + 8 * 2], rax
mov rax, 0x4010000000000000
mov [rdx + 8 * 3], rax

mov rax, 0x4022000000000000 ; 9.0
mov [rdx + 8 * 4], rax
mov rax, 0x4022000000000000
mov [rdx + 8 * 5], rax

mov rax, 0x4030000000000000 ; 16.0
mov [rdx + 8 * 6], rax
mov rax, 0x4030000000000000
mov [rdx + 8 * 7], rax

mov rax, 0x4039000000000000 ; 25.0
mov [rdx + 8 * 8], rax
mov rax, 0x4039000000000000
mov [rdx + 8 * 9], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 2]
movapd xmm2, [rdx + 8 * 4]
movapd xmm3, [rdx + 8 * 6]
movapd xmm4, [rdx + 8 * 8]
movapd xmm5, [rdx + 8 * 8]
movapd xmm6, [rdx + 8 * 8]
movapd xmm7, [rdx + 8 * 8]

sqrtpd xmm0, xmm0
sqrtpd xmm1, xmm1
sqrtpd xmm2, xmm2
sqrtpd xmm3, xmm3

sqrtpd xmm4, [rdx + 8 * 0]
sqrtpd xmm5, [rdx + 8 * 2]
sqrtpd xmm6, [rdx + 8 * 4]
sqrtpd xmm7, [rdx + 8 * 6]

hlt
