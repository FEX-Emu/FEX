%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x41424344FE2A4748"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x41424344C5464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

sar word [rdx + 8 * 0 + 2], 5

mov rax, [rdx + 8 * 0]

hlt
