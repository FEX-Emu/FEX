%ifdef CONFIG
{
  "RegData": {
    "MM4": "0x0",
    "MM5": "0xFEFEFEFEFEFEFEFE",
    "MM6": "0x0202020202020202",
    "MM7": "0xFE000200FE02FE00"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x0000000000000000
mov [rdx + 8 * 0], rax
mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 1], rax
mov rax, 0x0101010101010101
mov [rdx + 8 * 2], rax
mov rax, 0x0202020202020202
mov [rdx + 8 * 3], rax
mov rax, 0xFF000100FF01FF00
mov [rdx + 8 * 4], rax

movq mm0, [rdx + 8 * 0]
movq mm1, [rdx + 8 * 1]
movq mm2, [rdx + 8 * 2]
movq mm3, [rdx + 8 * 4]

movq mm4, [rdx + 8 * 3]
movq mm5, [rdx + 8 * 3]
movq mm6, [rdx + 8 * 3]
movq mm7, [rdx + 8 * 3]

; Test with full zero
psignb mm4, mm0

; Test with full negative
psignb mm5, mm1

; Test with full positive
psignb mm6, mm2

; Test a mix
psignb mm7, mm3

hlt
