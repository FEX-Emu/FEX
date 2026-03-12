%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x5152535455565758",
    "RDX": "0x5858585858585858",
    "RDI": "0xDFFFFFFF",
    "RSI": "0xE0000000"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx], rax
mov rax, 0x5152535455565758
mov [rdx + 8], rax

; Deliberately overlapping source and destination
lea rdi, [rdx + 7]
lea rsi, [rdx + 8]

std
mov rcx, 8
rep movsb ; rdi <- rsi

mov rdx, [rdx]
hlt
