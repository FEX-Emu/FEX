%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "XMM0":  ["0x414243447f800000", "0x5152535455565758"],
    "XMM1":  ["0x41424344ff800000", "0x5152535455565758"],
    "XMM2":  ["0x4142434400000000", "0x5152535455565758"]
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

section .text
global _start

_start:
movapd xmm0, [rel arg1]
movapd xmm1, [rel arg2]
movapd xmm2, [rel arg3]

rsqrtss xmm0, xmm0
rsqrtss xmm1, xmm1
rsqrtss xmm2, xmm2

; The last comparison returns nan so we need to check the 
; result manually
ucomiss xmm2, xmm2
setp al ; sets al to 1 if xmm2 is nan
xor esi, esi
pinsrd xmm2, esi, 0 ; inserts 0 in place of nan to test other bits
hlt

section .data
align 32
arg1:
dq 0x4142434400000000 ; 0.0, result is inf
dq 0x5152535455565758

arg2:
dq 0x4142434480000000 ; -0.0, result is -inf
dq 0x5152535455565758

arg3:
dq 0x41424344c0800000 ; -4.0, result is nan
dq 0x5152535455565758
