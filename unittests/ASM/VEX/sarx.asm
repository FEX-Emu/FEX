%ifdef CONFIG
{
  "RegData": {
      "RAX": "0xF800000000000000",
      "RBX": "4",
      "RCX": "0xFFFFFFFFFFFFFFFF",
      "RDX": "127",
      "RSI": "63",
      "RDI": "0x00000000FFFFFFFF"
  },
  "HostFeatures": ["BMI2"]
}
%endif

; Trivial right shift
mov rax, 0x8000000000000000
mov rbx, 4
sarx rax, rax, rbx

; This is really a shift by 63. This just ensures we properly
; mask the shift value according to the ISA manual.
mov rcx, 0x8000000000000000
mov rdx, 127
sarx rcx, rcx, rdx

; This is really a shift by 31. This just ensures we properly
; mask the shift value according to the ISA manual.
mov edi, 0x80000000
mov esi, 63
sarx edi, edi, esi

hlt
