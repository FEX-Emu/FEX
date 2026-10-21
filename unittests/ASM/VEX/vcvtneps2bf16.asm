%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xff800000ffff8142", "0", "0", "0"],
    "XMM1": ["0xff800000ffff8142", "0x8000cf853f857f80", "0", "0"],
    "XMM2": ["0xff800000ffff8142", "0", "0", "0"],
    "XMM3": ["0xff800000ffff8142", "0x8000cf853f857f80", "0", "0"],
    "XMM4": ["0xffffffff81420000", "0xff80000000000001", "0x3f8555557f800000", "0x80000000cf855555"]
  },
  "HostFeatures": ["AVX_NE_CONVERT"]
}
%endif

; Test float32->bfloat16 convert.
; Rounding mode is always `Round to nearest (even)`
; ARM alt-mode float handling ensures this is true when converted to BFCVTN.
; Convert 4 or 8 float numbers to bfloat16.

vmovups ymm0, [rel .data_junk]
vmovups ymm1, [rel .data_junk]
vmovups ymm2, [rel .data_junk]
vmovups ymm3, [rel .data_junk]

; And one register for testing register variants.
vmovups ymm4, [rel .data]

; nasm fails to support encoding these as vex.
db 0xc4, 0xe2, 0x7a, 0x72, 0x05, 0xcf, 0x0f, 0x00, 0x00 ; {vex} vcvtneps2bf16 xmm0, [rel .data]
db 0xc4, 0xe2, 0x7e, 0x72, 0x0d, 0xc6, 0x0f, 0x00, 0x00 ; {vex} vcvtneps2bf16 xmm1, yword [rel .data]
db 0xc4, 0xe2, 0x7a, 0x72, 0xd4 ; {vex} vcvtneps2bf16 xmm2, xmm4
db 0xc4, 0xe2, 0x7e, 0x72, 0xdc ; {vex} vcvtneps2bf16 xmm3, ymm4

hlt

align 4096
.data:
dd 0x81420000 ; Regular number
dd 0xFFFFFFFF ; NaN
dd 0x00000001 ; Subnormal
dd 0xFF800000 ; -infinity
dd 0x7F800000 ; +infinity
dd 0x3f855555 ; Positive large number
dd 0xcf855555 ; Negative large number
dd 0x80000000 ; Negative zero

.data_junk:
dq 0x4142434445464748
dq 0x5152535455565758
dq 0x6162636465666768
dq 0x7172737475767778
