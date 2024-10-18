%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM3":  ["0x1111111111111111", "0x3333333333333333", "0x5555555555555555", "0x7777777777777777"],
    "XMM4":  ["0x1111111111111111", "0x3333333333333333", "0x0000000000000000", "0x0000000000000000"],
    "XMM5":  ["0xAAAAAAAAAAAAAAAA", "0xCCCCCCCCCCCCCCCC", "0xEEEEEEEEEEEEEEEE", "0x9999999999999999"],
    "XMM6":  ["0xAAAAAAAAAAAAAAAA", "0xCCCCCCCCCCCCCCCC", "0x0000000000000000", "0x0000000000000000"],
    "XMM7":  ["0x1111111111111111", "0xCCCCCCCCCCCCCCCC", "0x5555555555555555", "0x9999999999999999"],
    "XMM8":  ["0x1111111111111111", "0xCCCCCCCCCCCCCCCC", "0x0000000000000000", "0x0000000000000000"],
    "XMM9":  ["0xAAAAAAAAAAAAAAAA", "0x3333333333333333", "0xEEEEEEEEEEEEEEEE", "0x7777777777777777"],
    "XMM10": ["0xAAAAAAAAAAAAAAAA", "0x3333333333333333", "0x0000000000000000", "0x0000000000000000"],
    "XMM11": ["0x1111111111111111", "0x3333333333333333", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

vmovaps ymm0, [rdx]
vmovaps ymm1, [rdx + 32]
vmovaps ymm2, [rel .mask_all]

; Select all ymm1
vblendvpd ymm3, ymm0, ymm1, ymm2
vblendvpd xmm4, xmm0, xmm1, xmm2

; Select all ymm0
vmovaps ymm2, [rel .mask_none]
vblendvpd ymm5, ymm0, ymm1, ymm2
vblendvpd xmm6, xmm0, xmm1, xmm2

; Interleaved selection from ymm1 and ymm0
vmovaps ymm2, [rel .mask_interleave1]
vblendvpd ymm7, ymm0, ymm1, ymm2
vblendvpd xmm8, xmm0, xmm1, xmm2

; Interleaved selection from ymm0 and ymm1
vmovaps ymm2, [rel .mask_interleave2]
vblendvpd ymm9,  ymm0, ymm1, ymm2
vblendvpd xmm10, xmm0, xmm1, xmm2

; Select all ymm0, with data in upper-bits
vmovaps ymm11, [rel .data_bad]
vmovaps ymm2, [rel .mask_all]
vblendvpd xmm11, xmm0, xmm1, xmm2

hlt

align 32
.data:
dq 0xAAAAAAAAAAAAAAAA
dq 0xCCCCCCCCCCCCCCCC
dq 0xEEEEEEEEEEEEEEEE
dq 0x9999999999999999

dq 0x1111111111111111
dq 0x3333333333333333
dq 0x5555555555555555
dq 0x7777777777777777

.mask_all:
dq 0xFFFFFFFFFFFFFFFF
dq 0xFFFFFFFFFFFFFFFF
dq 0xFFFFFFFFFFFFFFFF
dq 0xFFFFFFFFFFFFFFFF

.mask_none:
dq 0x0000000000000000
dq 0x0000000000000000
dq 0x0000000000000000
dq 0x0000000000000000

.mask_interleave1:
dq 0x8000000000000000
dq 0x0000000000000000
dq 0x8000000000000000
dq 0x0000000000000000

.mask_interleave2:
dq 0x0000000000000000
dq 0x8000000000000000
dq 0x0000000000000000
dq 0x8000000000000000

.data_bad:
dq 0x3132333435363738
dq 0x4142434445464748
dq 0x5152535455565758
dq 0x6162636465666768
