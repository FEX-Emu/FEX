%ifdef CONFIG
{
  "HostFeatures": ["AVX", "AVX_VNNI"],
  "RegData": {
    "XMM4": ["0x800000000000000B", "0xFFFF000180000002", "0x0000000000000000", "0x0000000000000000"],
    "XMM5": ["0x0000000000000040", "0x400080000002FFFE", "0x0000000000000000", "0x0000000000000000"],
    "XMM6": ["0x00008000FFFBFFFE", "0xFFFE8001FFFD8001", "0x800100001C173F20", "0x0001800000008000"],
    "XMM7": ["0x800000000000000B", "0xFFFF000180000002", "0x8765C322E02B0AC8", "0x000100107FFFFFFE"]
  }
}
%endif

vmovdqa ymm0, [rel src1]
vmovdqa ymm1, [rel src2]
vmovdqa ymm4, [rel accumulator]
vmovdqa ymm5, [rel accumulator]
vmovdqa ymm6, [rel src2]
vmovdqa ymm7, [rel accumulator]
lea rdx, [rel src2]

; Hand assembled instructions, since NASM will only output
; the EVEX encodings for these instructions
db 0xC4, 0xE2, 0x79, 0x52, 0xE1 ; vpdpwssd xmm4, xmm0, xmm1
db 0xC4, 0xE2, 0x51, 0x52, 0x2A ; vpdpwssd xmm5, xmm5, [rdx]
db 0xC4, 0xE2, 0x7D, 0x52, 0xF6 ; vpdpwssd ymm6, ymm0, ymm6
db 0xC4, 0xE2, 0x7D, 0x52, 0x3A ; vpdpwssd ymm7, ymm0, [rdx]

hlt

align 32
accumulator:
dd 0x00000010
dd 0x00000000
dd 0x00020000
dd 0x80000000
dd 0x12345678
dd 0x87654321
dd 0xFFFFFFFE
dd 0x80000010

src1:
dw 1, 2
dw -32768, -32768
dw 32767, 32767
dw -32768, 32767
dw 12345, -23456
dw -1, -2
dw -32768, -32768
dw 32767, 32767

src2:
dw 3, -4
dw -32768, -32768
dw 32767, 32767
dw -32768, 32767
dw -30000, 20000
dw 32767, -32768
dw -32768, -32768
dw -32768, -32768
