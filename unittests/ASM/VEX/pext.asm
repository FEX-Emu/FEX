%ifdef CONFIG
{
  "RegData": {
      "RAX": "0x12345678",
      "RBX": "0xFF00FFF0",
      "RCX": "0x00012567",
      "RDX": "0x1234567812345678",
      "RSI": "0xFF00FF00FF00FF00",
      "RDI": "0x12561256"
  }
}
%endif

; 32-bit
mov eax, 0x12345678
mov ebx, 0xFF00FFF0
pext ecx, eax, ebx

; 64-bit
mov rdx, 0x1234567812345678
mov rsi, 0xFF00FF00FF00FF00
pext rdi, rdx, rsi

hlt