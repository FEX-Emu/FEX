%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x0001000081420000", "0x454600007f800000", "0", "0"],
    "XMM1": ["0x0001000081420000", "0x454600007f800000", "0x5556000041420000", "0x6566000051520000"]
  },
  "HostFeatures": ["AVX_NE_CONVERT"]
}
%endif

; Test float16 convert vector-even.
; Does not round, is exact.
vmovups ymm0, [rel .data_junk]
vmovups ymm1, [rel .data_junk]

; Old nasm can't encode these.
db 0xc4, 0xe2, 0x79, 0xb0, 0x05, 0xe7, 0x0f, 0x00, 0x00 ; vcvtneeph2ps xmm0, [rel .data]
db 0xc4, 0xe2, 0x7d, 0xb0, 0x0d, 0xde, 0x0f, 0x00, 0x00 ; vcvtneeph2ps ymm1, [rel .data]
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
