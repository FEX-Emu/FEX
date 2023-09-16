%ifdef CONFIG
{
  "RegData": {
      "RAX": "0x12345678",
      "RBX": "0xFF00FFF0",
      "RCX": "0x00012567",
      "RDX": "0x1234567812345678",
      "RSI": "0xFF00FF00FF00FF00",
      "RDI": "0x12561256",
      "R8":  "0x1234567812345678",
      "R10": "0x12345678",
      "R11": "0x12345678",
      "R12": "0x00005678"
  },
  "HostFeatures": ["BMI2"]
}
%endif

; 32-bit
mov eax, 0x12345678
mov ebx, 0xFF00FFF0
pext ecx, eax, ebx

; 32-bit full mask
mov r10d,  0x12345678
mov r9d, 0xFFFFFFFF
pext r10d, r10d, r9d

; 32-bit half mask
mov r12d, 0x12345678
mov r9d, 0x0000FFFF
pext r12d, r12d, r9d

; 64-bit
mov rdx, 0x1234567812345678
mov rsi, 0xFF00FF00FF00FF00
pext rdi, rdx, rsi

; 64-bit full mask
mov r8, 0x1234567812345678
mov r9, 0xFFFFFFFFFFFFFFFF
pext r8, r8, r9

; 64-bit half mask
mov r11, 0x1234567812345678
mov r9,  0x00000000FFFFFFFF
pext r11, r11, r9

hlt
