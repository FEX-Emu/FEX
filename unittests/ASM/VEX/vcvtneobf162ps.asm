%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xff800000ffff0000", "0x4344000047480000", "0", "0"],
    "XMM1": ["0xff800000ffff0000", "0x4344000047480000", "0x5354000057580000", "0x6364000067680000"]
  },
  "HostFeatures": ["AVX_NE_CONVERT"]
}
%endif

; Test bfloat16 convert vector-odd.
; Does not round, is exact.
vmovups ymm0, [rel .data_junk]
vmovups ymm1, [rel .data_junk]

; Old nasm can't encode these.
db 0xc4, 0xe2, 0x7b, 0xb0, 0x05, 0xe7, 0x0f, 0x00, 0x00 ; vcvtneobf162ps xmm0, [rel .data]
db 0xc4, 0xe2, 0x7f, 0xb0, 0x0d, 0xde, 0x0f, 0x00, 0x00 ; vcvtneobf162ps ymm1, [rel .data]
hlt

align 4096
.data:
dw 0x8142 ; Regular number
.data2:
dw 0xFFFF ; NaN
.data3:
dw 0x0001 ; Subnormal
.data4:
dw 0xFF80 ; -infinity
.data5:
dw 0x7F80 ; +infinity

.data_junk:
dq 0x4142434445464748
dq 0x5152535455565758
dq 0x6162636465666768
dq 0x7172737475767778
