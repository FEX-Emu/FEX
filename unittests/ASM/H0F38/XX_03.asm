%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x7FFF7FFF7FFF7FFF",
    "MM1": "0x7FFF7FFF7FFF7FFF",
    "MM2": "0x7FFF7FFF7FFF7FFF",
    "MM3": "0x8000800080008000",
    "MM4": "0x800080007FFF7FFF",
    "MM5": "0x7FFF7FFF80008000",
    "MM6": "0x71836D874331472D"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x7F7F7F7F7F7F7F7F
mov [rdx + 8 * 2], rax
mov rax, 0x8080808080808080
mov [rdx + 8 * 3], rax

mov rax, 0x2119221823172416
mov [rdx + 8 * 4], rax
mov rax, 0x3941384237433644
mov [rdx + 8 * 5], rax

movq mm0, [rdx + 8 * 0]
movq mm1, [rdx + 8 * 1]
movq mm2, [rdx + 8 * 2]
movq mm3, [rdx + 8 * 3]
movq mm4, [rdx + 8 * 2]
movq mm5, [rdx + 8 * 3]
movq mm6, [rdx + 8 * 4]

phaddsw mm0, [rdx + 8 * 1]
phaddsw mm1, [rdx + 8 * 0]

phaddsw mm2, [rdx + 8 * 2]
phaddsw mm3, [rdx + 8 * 3]

phaddsw mm4, [rdx + 8 * 3]
phaddsw mm5, [rdx + 8 * 2]

phaddsw mm6, [rdx + 8 * 5]

hlt
