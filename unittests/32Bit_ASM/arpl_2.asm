%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFF_01FD",
    "RBX": "0xFFFF_0201",
    "RCX": "0xFFFF_03FE",
    "RDX": "0xFFFF_04FF",
    "RSI": "0xFFFF_0501",
    "RDI": "0xFFFF_06FC"
  },
  "Mode": "32BIT"
}
%endif

; Tests if ARPL copies or leaves alone the correct registers.
mov eax, 0xFFFF_01FC
mov ebx, 0xFFFF_0201
mov ecx, 0xFFFF_03FE
mov edx, 0xFFFF_04FF
mov esi, 0xFFFF_0501
mov edi, 0xFFFF_06FC

; Modified dst < src
arpl ax, bx

; Unmodified dst = src
arpl bx, si

; Unmodified dst > src
arpl cx, si

hlt
