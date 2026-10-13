%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x414243444546F848"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x414243444546C348
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

sar byte [rdx + 8 * 0 + 1], 3

mov rax, [rdx + 8 * 0]

hlt
