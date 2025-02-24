%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3f800000",
    "XMM0": ["0x4142434400000000", "0x5152535455565758"]
  }
}
%endif

section .text
global _start

_start:
xor esi, esi

movapd xmm0, [rel arg1]

pextrd [rel val], xmm0, 0
pinsrd xmm0, esi, 0

mov eax, [rel val]
hlt

section .bss
align 32
val resd 1

section .data
align 128
arg1: 
dq 0x414243443f800000
dq 0x5152535455565758