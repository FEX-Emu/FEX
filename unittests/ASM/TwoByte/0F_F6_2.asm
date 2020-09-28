%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x00000000000007F8"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x0
mov [rdx + 8 * 0], rax
mov rax, -1
mov [rdx + 8 * 1], rax

movq mm0, [rdx + 8 * 0]

psadbw mm0, [rdx + 8 * 1]

hlt
