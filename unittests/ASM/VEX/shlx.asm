%ifdef CONFIG
{
  "RegData": {
      "RAX": "0x10",
      "RBX": "4",
      "RCX": "0x8000000000000000",
      "RDX": "127",
      "RSI": "63",
      "RDI": "0x80000000"
  },
  "HostFeatures": ["BMI2"]
}
%endif

; Trivial left shift
mov rax, 1
mov rbx, 4
shlx rax, rax, rbx

; This is really a shift by 63. This just ensures we properly
; mask the shift value according to the ISA manual.
mov rcx, 1
mov rdx, 127
shlx rcx, rcx, rdx

; This is really a shift by 31. This just ensures we properly
; mask the shift value according to the ISA manual.
mov edi, 1
mov esi, 63
shlx edi, edi, esi

hlt
