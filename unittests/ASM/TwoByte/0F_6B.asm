%ifdef CONFIG
{
  "RegData": {
    "MM0": "0xFFFF8000FFFF8000",
    "MM1": "0xFFFF8000FFFF8000",
    "MM2": "0xFFFFFFFF80000000"
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

movq mm0, [rdx]
packssdw mm0, [rdx + 8 * 2]

movq mm1, [rdx]
movq mm2, [rdx + 8 * 2]
packssdw mm1, mm2

hlt
