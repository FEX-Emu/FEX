%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000000000004",
    "RBX": "0x0000000000000004",
    "RCX": "0x0000000000000006",
    "RDX": "0x0000000000000006",
    "XMM0": ["0x402f800000000000", "0x400c000000000000"],
    "XMM1": ["0x402f800000000000", "0x4035400000000000"],
    "XMM2": ["0x0000000045464748", "0"],
    "XMM3": ["0x4142434445464748", "0"],
    "XMM4": ["0x0000000041424344", "0"],
    "XMM5": ["0x0000000045464748", "0x5152535455565758"]
  },
  "Mode": "32BIT"
}
%endif

; FEX-Emu had a bug where 32-bit applications that relied on VEX.W would incorrectly handle widening behaviour.
; AVX instructions that use VEX.W wouldn't scale their element sizes correctly.
; Checks all instructions (skipping a few duplicates in the same class) that react to VEX.W.

vmovaps xmm0, [rel .data_xmm0]
vmovaps xmm1, [rel .data_xmm1]
vmovaps xmm2, [rel .data_xmm2]

; Affects all scalar FMA.
vfmadd132sd xmm0, xmm2, xmm1

; Affects all packed FMA.
vmovaps xmm1, [rel .data_xmm0]
vmovaps xmm2, [rel .data_xmm1]
vmovaps xmm3, [rel .data_xmm2]
vfmadd132pd xmm1, xmm3, xmm2

vmovaps xmm2, [rel .data_xmm0]
; Affects vcvttsd2si as well.
vcvtsd2si eax, xmm2
; This actually works on 32-bit, behaves like a 32-bit operation. Don't question it.
db 0xc4, 0xe1, 0xfb, 0x2d, 0xda; vcvtsd2si rbx, xmm2

vmovaps xmm2, [rel .data_6]
; Affects vcvttss2si as well.
vcvtss2si ecx, xmm2
; This actually works on 32-bit, behaves like a 32-bit operation. Don't question it.
db 0xc4, 0xe1, 0xfa, 0x2d, 0xd2  ; vcvtss2si rdx, xmm2

vmovaps xmm2, [rel .data_test]
vmovd dword [rel .data_temp], xmm2
vmovaps xmm2, [rel .data_temp]

vmovaps xmm3, [rel .data_test]
vmovq qword [rel .data_temp], xmm3
vmovaps xmm3, [rel .data_temp]

vpxor xmm7, xmm7, xmm7
vmovaps [rel .data_temp], xmm7

; vpextrq qword explicitly SIGILLs on 32-bit
vmovaps xmm4, [rel .data_test]
vpextrd dword [rel .data_temp], xmm4, 1
vmovaps xmm4, [rel .data_temp]

vmovaps [rel .data_temp], xmm7

; vpinsrq qword explicitly SIGILLs on 32-bit
vmovaps xmm5, [rel .data_test]
vpinsrd xmm5, dword [rel .data_temp + 8], 1

hlt

align 4096
.data_xmm0:
dq 0x400c000000000000, 0x400c000000000000
.data_xmm1:
dq 0x400c000000000000, 0x4012000000000000
.data_xmm2:
dq 0x400c000000000000, 0x4016000000000000

.data_test:
dq 0x4142434445464748, 0x5152535455565758

.data_6:
dd 6.0, 6.0, 6.0, 6.0

.data_temp:
dq 0, 0
