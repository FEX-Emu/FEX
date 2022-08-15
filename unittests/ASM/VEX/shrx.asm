%ifdef CONFIG
{
  "RegData": {
      "RAX": "0x0800000000000000",
      "RBX": "4",
      "RCX": "1",
      "RDX": "127",
      "RSI": "63",
      "RDI": "1"
  },
  "HostFeatures": ["BMI2"]
}
%endif

; Trivial right shift
mov rax, 0x8000000000000000
mov rbx, 4
shrx rax, rax, rbx

; This is really a shift by 63. This just ensures we properly
; mask the shift value according to the ISA manual.
mov rcx, 0x8000000000000000
mov rdx, 127
shrx rcx, rcx, rdx

; This is really a shift by 31. This just ensures we properly
; mask the shift value according to the ISA manual.
mov edi, 0x80000000
mov esi, 63
shrx edi, edi, esi

hlt
