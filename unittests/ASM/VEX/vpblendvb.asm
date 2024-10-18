%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM3":  ["0x1222324252627282", "0xAABBCCDDEEFF9900", "0x8070605040302010", "0x1020304050607080"],
    "XMM4":  ["0x1222324252627282", "0xAABBCCDDEEFF9900", "0x0000000000000000", "0x0000000000000000"],
    "XMM5":  ["0xAABBCCDDEEFF1122", "0x3344556677889900", "0x1020304050607080", "0x9585756555453525"],
    "XMM6":  ["0xAABBCCDDEEFF1122", "0x3344556677889900", "0x0000000000000000", "0x0000000000000000"],
    "XMM7":  ["0xAA22CC42EE621182", "0x33BB55DD77FF9900", "0x1070305050307010", "0x9520754055603580"],
    "XMM8":  ["0xAA22CC42EE621182", "0x33BB55DD77FF9900", "0x0000000000000000", "0x0000000000000000"],
    "XMM9":  ["0x12BB32DD52FF7222", "0xAA44CC66EE889900", "0x8020604040602080", "0x1085306550457025"],
    "XMM10": ["0x12BB32DD52FF7222", "0xAA44CC66EE889900", "0x0000000000000000", "0x0000000000000000"],
    "XMM11": ["0x1222324252627282", "0xaabbccddeeff9900", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

vmovaps ymm0, [rdx]
vmovaps ymm1, [rdx + 32]
vmovaps ymm2, [rel .mask_all]

; Select all ymm1
vpblendvb ymm3, ymm0, ymm1, ymm2
vpblendvb xmm4, xmm0, xmm1, xmm2

; Select all ymm0
vmovaps ymm2, [rel .mask_none]
vpblendvb ymm5, ymm0, ymm1, ymm2
vpblendvb xmm6, xmm0, xmm1, xmm2

; Interleaved selection from ymm1 and ymm0
vmovaps ymm2, [rel .mask_interleave1]
vpblendvb ymm7, ymm0, ymm1, ymm2
vpblendvb xmm8, xmm0, xmm1, xmm2

; Interleaved selection from ymm0 and ymm1
vmovaps ymm2, [rel .mask_interleave2]
vpblendvb ymm9,  ymm0, ymm1, ymm2
vpblendvb xmm10, xmm0, xmm1, xmm2

; Select all ymm0, with data in upper-bits
vmovaps ymm11, [rel .data_bad]
vmovaps ymm2, [rel .mask_all]
vpblendvb xmm11, xmm0, xmm1, xmm2

hlt

align 32
.data:
dq 0xAABBCCDDEEFF1122
dq 0x3344556677889900
dq 0x1020304050607080
dq 0x9585756555453525

dq 0x1222324252627282
dq 0xAABBCCDDEEFF9900
dq 0x8070605040302010
dq 0x1020304050607080

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
dq 0x0080008000800080
dq 0x0080008000800080
dq 0x0080008000800080
dq 0x0080008000800080

.mask_interleave2:
dq 0x8000800080008000
dq 0x8000800080008000
dq 0x8000800080008000
dq 0x8000800080008000

.data_bad:
dq 0x3132333435363738
dq 0x4142434445464748
dq 0x5152535455565758
dq 0x6162636465666768
