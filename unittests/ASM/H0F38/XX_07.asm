%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x0202020202020202",
    "MM1": "0x0202020202020202",
    "MM2": "0x0",
    "MM3": "0x0",
    "MM4": "0x0",
    "MM5": "0x0",
    "MM6": "0xFF01FF0100FF00FF"
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

phsubsw mm0, [rdx + 8 * 1]
phsubsw mm1, [rdx + 8 * 0]

phsubsw mm2, [rdx + 8 * 2]
phsubsw mm3, [rdx + 8 * 3]

phsubsw mm4, [rdx + 8 * 3]
phsubsw mm5, [rdx + 8 * 2]

phsubsw mm6, [rdx + 8 * 5]

hlt
