%ifdef CONFIG
{
  "RegData": {
      "RAX": "1",
      "RBX": "0xFF00000000000000",
      "RCX": "0x0100000000000000",
      "RDX": "1",
      "RSI": "0xFF000000",
      "RDI": "0x01000000"
  },
  "HostFeatures": ["BMI1"]
}
%endif

; Trivial test, this should result in 1.
mov rax, 11
blsi rax, rax

; Results in the lowest set bit (bit 56) being extracted
mov rbx, 0xFF00000000000000
mov rcx, 0
blsi rcx, rbx

; Same tests but with 32-bit registers

; Trivial test, this should result in 1.
mov edx, 11
blsi edx, edx

; Results in the lowest set bit (bit 24) being extracted
mov rsi, 0xFF000000
mov rdi, 0
blsi edi, esi

hlt
