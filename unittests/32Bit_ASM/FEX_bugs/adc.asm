%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x00000000fffffffe",
    "RBX": "0x0000000000000001"
  },
  "Mode": "32BIT"
}
%endif

; FEX had a bug where ADD or SUB with carry was generating results with garbage in the upper 32-bits.

mov eax, -1
mov ebx, -1
mov edx, -1

clc
adc eax, edx
adc eax, edx
adc eax, edx

clc
sbb ebx, edx
sbb ebx, edx
sbb ebx, edx
hlt
