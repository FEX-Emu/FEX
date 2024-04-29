%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xaeadacac9a9a41e5",
    "RBX": "0x6162636520238df8"
  }
}
%endif

; FEX had a bug with smaller than 32-bit operations corrupting sbb and adc results.
; A small test that tests both sbb and adc to ensure it returns data correctly.
; This was noticed in Final Fantasy 7 (steamid 39140) having broken rendering on the title screen.
mov rax, 0x4142434445464748
mov rbx, 0x5152535455565758
mov rcx, 0x6162636465666768

clc
sbb al, bl
sbb ax, bx
sbb eax, ebx
sbb rax, rbx

clc
adc bl, cl
adc bx, cx
adc ebx, ecx
adc rbx, rcx

hlt
