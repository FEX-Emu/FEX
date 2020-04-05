%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x00000040FFFF8000", "0x00000040FFFF8000"],
    "XMM1": ["0x00000040FFFF8000", "0x00000040FFFF8000"],
    "XMM2": ["0xFFFFFFFF80000000", "0x0000000000000040"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

; 32bit signed -> 16bit signed (saturated)
; input > 0x7FFF(SHRT_MAX, 32767) = 0x7FFF(SHRT_MAX, 32767)
; input < 0x8000(-32767) = 0x8000

mov rax, 0xFFFFFFFF80000000
mov [rdx + 8 * 0], rax
mov rax, 0x0000000000000040
mov [rdx + 8 * 1], rax

mov rax, 0xFFFFFFFF80000000
mov [rdx + 8 * 2], rax
mov rax, 0x0000000000000040
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx]
packssdw xmm0, [rdx + 8 * 2]

movapd xmm1, [rdx]
movapd xmm2, [rdx + 8 * 2]
packssdw xmm1, xmm2

hlt
