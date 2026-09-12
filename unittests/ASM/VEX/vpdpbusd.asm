%ifdef CONFIG
{
  "HostFeatures": ["AVX", "AVX_VNNI"],
  "RegData": {
    "XMM4": ["0xFFFFFF3100000006", "0x7FFF07008001F904", "0x0000000000000000", "0x0000000000000000"],
    "XMM5": ["0xFFFFFFFF00000020", "0x7FFFC08080013B03", "0x0000000000000000", "0x0000000000000000"],
    "XMM6": ["0x02FE0140FC03FDF7", "0x807F86807F817983", "0xFCFDFD0101FF8000", "0x807E82807F817983"],
    "XMM7": ["0xFFFFFF3100000006", "0x7FFF07008001F904", "0x87654123123456F8", "0x7FFE02108001F9F4"]
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
; the EVEX encodings for these instructions.
db 0xC4, 0xE2, 0x79, 0x50, 0xE1 ; vpdpbusd xmm4, xmm0, xmm1
db 0xC4, 0xE2, 0x51, 0x50, 0x2A ; vpdpbusd xmm5, xmm5, [rdx]
db 0xC4, 0xE2, 0x7D, 0x50, 0xF6 ; vpdpbusd ymm6, ymm0, ymm6
db 0xC4, 0xE2, 0x7D, 0x50, 0x3A ; vpdpbusd ymm7, ymm0, [rdx]

hlt

align 32
accumulator:
dd 0x00000010
dd 0xFFFFFFF0
dd 0x7FFFFF00
dd 0x80000100
dd 0x12345678
dd 0x87654321
dd 0x7FFFFFF0
dd 0x80000010

src1:
db 1, 2, 3, 4
db 255, 128, 64, 32
db 255, 255, 255, 255
db 200, 150, 100, 50
db 0, 1, 254, 255
db 17, 34, 51, 68
db 255, 255, 255, 255
db 255, 255, 255, 255

src2:
db 1, -2, 3, -4
db -1, 1, -2, 2
db 127, 127, 127, 127
db -128, -128, -128, -128
db -128, 127, -1, 1
db -1, -2, -3, -4
db 127, 127, 127, 127
db -128, -128, -128, -128
