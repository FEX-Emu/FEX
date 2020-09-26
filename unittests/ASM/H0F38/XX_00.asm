%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x4848484848484848",
    "MM1": "0x0",
    "MM2": "0x0",
    "MM3": "0x4847464544434241"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x0
mov [rdx + 8 * 1], rax
mov rax, -1
mov [rdx + 8 * 2], rax
mov rax, 0x8080808080808080
mov [rdx + 8 * 3], rax
mov rax, 0x0001020304050607
mov [rdx + 8 * 4], rax

movq mm0, [rdx + 8 * 0]
movq mm1, [rdx + 8 * 0]
movq mm2, [rdx + 8 * 0]
movq mm3, [rdx + 8 * 0]

pshufb mm0, [rdx + 8 * 1]
pshufb mm1, [rdx + 8 * 2]
pshufb mm2, [rdx + 8 * 3]
pshufb mm3, [rdx + 8 * 4]

hlt
