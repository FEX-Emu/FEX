%ifdef CONFIG
{
  "RegData": {
      "RAX": "10",
      "RBX": "0xFF00000000000000",
      "RCX": "0xFE00000000000000",
      "RDX": "10",
      "RSI": "0xFF000000",
      "RDI": "0xFE000000"
  },
  "HostFeatures": ["BMI1"]
}
%endif

; Trivial test, this should result in 10.
mov rax, 11
blsr rax, rax

; Results in 0xFE00000000000000 being placed into RCX
mov rbx, 0xFF00000000000000
blsr rcx, rbx

; Same tests but with 32-bit registers

; Trivial test, this should result in 10.
mov edx, 11
blsr edx, edx

; Results in 0xFE000000 being placed in EDI
mov rsi, 0xFF000000
blsr edi, esi

hlt
