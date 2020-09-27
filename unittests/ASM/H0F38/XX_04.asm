%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x0",
    "MM1": "0xFE02FE02FE02FE02",
    "MM2": "0x7E027E027E027E02",
    "MM3": "0x7FFF7FFF7FFF7FFF",
    "MM4": "0x0D130E5F0EB90F99",
    "MM5": "0x2D132E5F2FB93099",
    "MM6": "0x483B48E649914A3C",
    "MM7": "0x283B28E629912A3C"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x0
mov [rdx + 8 * 0], rax

mov rax, -1
mov [rdx + 8 * 1], rax

mov rax, 0x7F7F7F7F7F7F7F7F
mov [rdx + 8 * 2], rax

mov rax, 0x8141824383448445
mov [rdx + 8 * 6], rax
mov rax, 0x21F223F323F424F5
mov [rdx + 8 * 7], rax

mov rax, 0xE251E352E453E554
mov [rdx + 8 * 8], rax
mov rax, 0x71A972A873A774A6
mov [rdx + 8 * 9], rax

; Zero
movq mm0, [rdx + 8 * 0]
pmaddubsw mm0, [rdx + 8 * 0]

; -1
movq mm1, [rdx + 8 * 1]
pmaddubsw mm1, [rdx + 8 * 1]

; 127
movq mm2, [rdx + 8 * 2]
pmaddubsw mm2, [rdx + 8 * 2]

; 255 and 127
movq mm3, [rdx + 8 * 1]
pmaddubsw mm3, [rdx + 8 * 2]

; Mixture
movq mm4, [rdx + 8 * 6]
pmaddubsw mm4, [rdx + 8 * 7]

movq mm5, [rdx + 8 * 7]
pmaddubsw mm5, [rdx + 8 * 6]

movq mm6, [rdx + 8 * 8]
pmaddubsw mm6, [rdx + 8 * 9]

movq mm7, [rdx + 8 * 9]
pmaddubsw mm7, [rdx + 8 * 8]

hlt
