%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xc90fdaa22168c235", "0x4000"],
    "XMM1": ["0x8000000000000000", "0x4005"],
    "XMM2": ["0x8000000000000000", "0x4004"],
    "XMM3": ["0x8000000000000000", "0x4003"],
    "XMM4": ["0x8000000000000000", "0x4002"],
    "XMM5": ["0x8000000000000000", "0x4001"],
    "XMM6": ["0x8000000000000000", "0x4000"],
    "XMM7": ["0x0000000000000000", "0x0000"],
    "MM0":  ["0xc90fdaa22168c235", "0x4000"],
    "MM1":  ["0x8000000000000000", "0x4005"],
    "MM2":  ["0x8000000000000000", "0x4004"],
    "MM3":  ["0x8000000000000000", "0x4003"],
    "MM4":  ["0x8000000000000000", "0x4002"],
    "MM5":  ["0x8000000000000000", "0x4001"],
    "MM6":  ["0x8000000000000000", "0x4000"],
    "MM7":  ["0x0000000000000000", "0x0000"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [rel .data]

fldz
fild word [edx + 2 * 1]
fild word [edx + 2 * 2]
fild word [edx + 2 * 3]
fild word [edx + 2 * 4]
fild word [edx + 2 * 5]
fild word [edx + 2 * 6]
fldpi

o16 fnsave [edx]

fldpi
fldpi
fldpi
fldpi
fldpi
fldpi
fldpi
fldpi

o16 frstor [edx]

movups xmm0, [edx + (0xE + 10 * 0)]
movups xmm1, [edx + (0xE + 10 * 1)]
movups xmm2, [edx + (0xE + 10 * 2)]
movups xmm3, [edx + (0xE + 10 * 3)]
movups xmm4, [edx + (0xE + 10 * 4)]
movups xmm5, [edx + (0xE + 10 * 5)]
movups xmm6, [edx + (0xE + 10 * 6)]
movups xmm7, [edx + (0xE + 10 * 7)]

pslldq xmm0, 6
psrldq xmm0, 6

pslldq xmm1, 6
psrldq xmm1, 6

pslldq xmm2, 6
psrldq xmm2, 6

pslldq xmm3, 6
psrldq xmm3, 6

pslldq xmm4, 6
psrldq xmm4, 6

pslldq xmm5, 6
psrldq xmm5, 6

pslldq xmm6, 6
psrldq xmm6, 6

pslldq xmm7, 6
psrldq xmm7, 6

hlt

align 4096
.data:
dw 0
dw 2
dw 4
dw 8
dw 16
dw 32
dw 64
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
