%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "0x8182838485868788",
    "RCX": "0x7172737475767778",
    "RDX": "0x5152535455565758",
    "RSP": "0x6162636465666768",
    "RSI": "0x2122232425262728",
    "RDI": "3",
    "RBP": "0x1e"
  }
}
%endif

; FEX-Emu had a bug where BSR and BSF prefixed with repne didn't exist.
; Documentation says that these only work without a prefix.
; With `rep` prefix these change instructions, but with `repne` they still behave like BSF/BSR.
mov rax, 0
mov rbx, 0x8182838485868788
mov rcx, 0x7172737475767778
mov rsp, 0x6162636465666768
mov rsi, 0x2122232425262728
mov rdi, 0x3132333435363738
mov rbp, 0x4142434445464748
mov rdx, 0x5152535455565758

repne bsr ebp, edx
repne bsf edi, edx

setz al
hlt
