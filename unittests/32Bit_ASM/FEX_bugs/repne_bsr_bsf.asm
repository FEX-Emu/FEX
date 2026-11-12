%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "0x81828384",
    "RCX": "0x71727374",
    "RDX": "0x51525354",
    "RSP": "0x61626364",
    "RSI": "0x21222324",
    "RDI": "2",
    "RBP": "0x1e"
  },
  "Mode": "32BIT"
}
%endif

; FEX-Emu had a bug where BSR and BSF prefixed with repne didn't exist.
; Documentation says that these only work without a prefix.
; With `rep` prefix these change instructions, but with `repne` they still behave like BSF/BSR.
mov eax, 0
mov ebx, 0x81828384
mov ecx, 0x71727374
mov esp, 0x61626364
mov esi, 0x21222324
mov edi, 0x31323334
mov ebp, 0x41424344
mov edx, 0x51525354

repne bsr ebp, edx
repne bsf edi, edx

setz al
hlt
