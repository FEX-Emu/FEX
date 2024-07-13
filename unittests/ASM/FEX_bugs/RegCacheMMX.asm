%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x5152535455565758", "0"]
  }
}
%endif

fninit
; Load all test values
fld tword [rel .test_value]
fld tword [rel .test_value]
fld tword [rel .test_value]
fld tword [rel .test_value]
fld tword [rel .test_value]
fld tword [rel .test_value]
fld tword [rel .test_value]
fld tword [rel .test_value]

; Setup for MMX usage
emms

; Load XMM value
movups xmm0, [rel .test_xmm_value]

; Load MMX value
movq mm0, [rel .test_mmx_value]

jmp .test
.test:
; Move MMX register in to XMM
; Should set the upper 64-bits of xmm0 to zero
movq2dq xmm0, mm0

hlt
align 32

.test_value:
dq 0x4142434445464748
dw 0x7fff

.test_mmx_value:
dq 0x5152535455565758

.test_xmm_value:
dq 0x6162636465666768
dq 0x7172737475767778
