%ifdef CONFIG
{
  "RegData": {
    "MM4": "0x0",
    "MM5": "0xFBFEFBFEFBFEFBFE",
    "MM6": "0x0402040204020402",
    "MM7": "0xFBFE04020000FBFE"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x0000000000000000
mov [rdx + 8 * 0], rax
mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 1], rax
mov rax, 0x0001000100010001
mov [rdx + 8 * 2], rax
mov rax, 0x0402040204020402
mov [rdx + 8 * 3], rax
mov rax, 0xFFFF00010000FFFF
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
psignw mm4, mm0

; Test with full negative
psignw mm5, mm1

; Test with full positive
psignw mm6, mm2

; Test a mix
psignw mm7, mm3

hlt
