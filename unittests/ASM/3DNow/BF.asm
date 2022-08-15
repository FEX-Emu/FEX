%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x2179b0697d5378c4",
    "MM1": "0x1ed68638699d35ca",
    "MM2": "0x165c42291f28194c",
    "MM3": "0x2179b0697d5378c4",
    "MM4": "0x1ed68638699d35ca",
    "MM5": "0x165c42291f28194c"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

mov rdx, 0xe0000000

mov rax, 0x2bb883523d4f3197
mov [rdx + 8 * 0], rax
mov rax, 0x1246c77764260189
mov [rdx + 8 * 1], rax

mov rax, 0x163add80bc57bef1
mov [rdx + 8 * 2], rax
mov rax, 0x64d615e5b405a306
mov [rdx + 8 * 3], rax

mov rax, 0x11f4881d94eb39fc
mov [rdx + 8 * 4], rax
mov rax, 0xa9162248f2d0a23a
mov [rdx + 8 * 5], rax

mov rax, 0x0
mov [rdx + 8 * 6], rax
mov rax, 0x0
mov [rdx + 8 * 7], rax

movq mm0, [rdx + 8 * 0]
movq mm1, [rdx + 8 * 0]
movq mm2, [rdx + 8 * 0]
movq mm3, [rdx + 8 * 0]
movq mm4, [rdx + 8 * 0]
movq mm5, [rdx + 8 * 0]
movq mm6, [rdx + 8 * 2]
movq mm7, [rdx + 8 * 4]

pavgusb mm0, mm6
pavgusb mm1, mm7

movq mm7, [rdx + 8 * 6]
pavgusb mm2, mm7

pavgusb mm3, [rdx + 8 * 2]
pavgusb mm4, [rdx + 8 * 4]
pavgusb mm5, [rdx + 8 * 6]

hlt
