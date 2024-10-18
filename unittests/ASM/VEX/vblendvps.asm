%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM3":  ["0x1111111122222222", "0x3333333344444444", "0x5555555566666666", "0x7777777788888888"],
    "XMM4":  ["0x1111111122222222", "0x3333333344444444", "0x0000000000000000", "0x0000000000000000"],
    "XMM5":  ["0xAAAAAAAABBBBBBBB", "0xCCCCCCCCDDDDDDDD", "0xEEEEEEEEFFFFFFFF", "0x9999999988888888"],
    "XMM6":  ["0xAAAAAAAABBBBBBBB", "0xCCCCCCCCDDDDDDDD", "0x0000000000000000", "0x0000000000000000"],
    "XMM7":  ["0xAAAAAAAA22222222", "0xCCCCCCCC44444444", "0xEEEEEEEE66666666", "0x9999999988888888"],
    "XMM8":  ["0xAAAAAAAA22222222", "0xCCCCCCCC44444444", "0x0000000000000000", "0x0000000000000000"],
    "XMM9":  ["0x11111111BBBBBBBB", "0x33333333DDDDDDDD", "0x55555555FFFFFFFF", "0x7777777788888888"],
    "XMM10": ["0x11111111BBBBBBBB", "0x33333333DDDDDDDD", "0x0000000000000000", "0x0000000000000000"],
    "XMM11": ["0x1111111122222222", "0x3333333344444444", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

vmovaps ymm0, [rdx]
vmovaps ymm1, [rdx + 32]
vmovaps ymm2, [rel .mask_all]

; Select all ymm1
vblendvps ymm3, ymm0, ymm1, ymm2
vblendvps xmm4, xmm0, xmm1, xmm2

; Select all ymm0
vmovaps ymm2, [rel .mask_none]
vblendvps ymm5, ymm0, ymm1, ymm2
vblendvps xmm6, xmm0, xmm1, xmm2

; Interleaved selection from ymm1 and ymm0
vmovaps ymm2, [rel .mask_interleave1]
vblendvps ymm7, ymm0, ymm1, ymm2
vblendvps xmm8, xmm0, xmm1, xmm2

; Interleaved selection from ymm0 and ymm1
vmovaps ymm2, [rel .mask_interleave2]
vblendvps ymm9,  ymm0, ymm1, ymm2
vblendvps xmm10, xmm0, xmm1, xmm2

; Select all ymm0, with data in upper-bits
vmovaps ymm11, [rel .data_bad]
vmovaps ymm2, [rel .mask_all]
vblendvps xmm11, xmm0, xmm1, xmm2

hlt

align 32
.data:
dq 0xAAAAAAAABBBBBBBB
dq 0xCCCCCCCCDDDDDDDD
dq 0xEEEEEEEEFFFFFFFF
dq 0x9999999988888888

dq 0x1111111122222222
dq 0x3333333344444444
dq 0x5555555566666666
dq 0x7777777788888888

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
dq 0x0000000080000000
dq 0x0000000080000000
dq 0x0000000080000000
dq 0x0000000080000000

.mask_interleave2:
dq 0x8000000000000000
dq 0x8000000000000000
dq 0x8000000000000000
dq 0x8000000000000000

.data_bad:
dq 0x3132333435363738
dq 0x4142434445464748
dq 0x5152535455565758
dq 0x6162636465666768
