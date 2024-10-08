%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464747"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax

dec byte [rdx + 8 * 0 + 0]
mov rax, [rdx + 8 * 0]

hlt
