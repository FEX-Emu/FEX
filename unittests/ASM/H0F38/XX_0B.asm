%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x00830087008B008F",
    "MM1": "0x0100FF0000FF0100"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x8001800280038004
mov [rdx + 8 * 1], rax
mov rax, 0x0101010101010101
mov [rdx + 8 * 2], rax
mov rax, 0xFF000100FF01FF00
mov [rdx + 8 * 3], rax

movq mm0, [rdx + 8 * 0]
movq mm1, [rdx + 8 * 1]

pmulhrsw mm0, [rdx + 8 * 2]

pmulhrsw mm1, [rdx + 8 * 3]

hlt
