%ifdef CONFIG
{
  "RegData": {
      "RAX": "1",
      "RBX": "0xFF00000000000000",
      "RCX": "0x01FFFFFFFFFFFFFF",
      "RDX": "1",
      "RSI": "0xFF000000",
      "RDI": "0x01FFFFFF"
  },
  "HostFeatures": ["BMI1"]
}
%endif

; Trivial test, this should result in 1.
mov rax, 11
blsmsk rax, rax

; Results in 0x01FFFFFFFFFFFFFF being placed into RCX
mov rbx, 0xFF00000000000000
blsmsk rcx, rbx

; Same tests but with 32-bit registers

; Trivial test, this should result in 1.
mov edx, 11
blsmsk edx, edx

; Results in 0x01FFFFFF being placed in EDI
mov rsi, 0xFF000000
blsmsk edi, esi

hlt
