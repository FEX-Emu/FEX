%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x5152535455565758",
    "RDX": "0x5152535455565748",
    "RDI": "0xE0000009",
    "RSI": "0xE0000008"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx], rax
mov rax, 0x5152535455565758
mov [rdx + 8], rax

; Deliberately overlapping source and destination
lea rdi, [rdx + 1]
lea rsi, [rdx]

cld
mov rcx, 8
rep movsb ; rdi <- rsi

mov rdx, [rdx + 8]
hlt
