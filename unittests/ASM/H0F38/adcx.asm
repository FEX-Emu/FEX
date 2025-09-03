%ifdef CONFIG
{
  "RegData": {
      "RAX": "2",
      "RBX": "3",
      "RCX": "1",
      "RDX": "2",
      "RSI": "1",
      "RDI": "3"
  },
  "HostFeatures": ["ADX"]
}
%endif

; Test with no carry
mov rax, 1
clc
adcx rax, rax

; Test with carry
mov rcx, 1
mov rbx, 1
stc
adcx rbx, rcx

; 32-bit registers

; Test with no carry
mov edx, 1
clc
adcx edx, edx

; Test with carry
mov esi, 1
mov edi, 1
stc
adcx edi, esi

hlt
