%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0xd9b11c39ec002fd9", "0x3ffe"],
    "XMM1":  ["0xd9b11c39ec002fd9", "0x3ffe"],
    "XMM2":  ["0xd9b11c39ec002fd9", "0x3ffe"],
    "XMM3":  ["0xd9b11c39ec002fd8", "0x3ffe"],
    "XMM4":  ["0xd9b11c39ec002fd8", "0x3ffe"],
    "XMM5":  ["0xd9b11c39ec002fd8", "0x3ffe"],
    "XMM6":  ["0xd9b11c39ec002fd9", "0x3ffe"],
    "XMM7":  ["0xd9b11c39ec002fd9", "0x3ffe"],
    "XMM8":  ["0xd9b11c39ec002fd9", "0x3ffe"],
    "XMM9":  ["0xd9b11c39ec002fd8", "0x3ffe"],
    "XMM10":  ["0xd9b11c39ec002fd8", "0x3ffe"],
    "XMM11":  ["0xd9b11c39ec002fd8", "0x3ffe"]
  }
}
%endif

%include "x87cw.mac"

mov rsp, 0xe000_1000

finit ; enters x87 state

; 80-bit mode, round-nearest
set_cw_precision_rounding x87_prec_80, x87_round_nearest
fld tword [rel .source_1]
fsin
fstp tword [rel .result_1]

; 64-bit mode, round-nearest
set_cw_precision_rounding x87_prec_64, x87_round_nearest
fld tword [rel .source_1]
fsin
fstp tword [rel .result_2]

; 32-bit mode, round-nearest
set_cw_precision_rounding x87_prec_32, x87_round_nearest
fld tword [rel .source_1]
fsin
fstp tword [rel .result_3]

; 80-bit mode, round-down
set_cw_precision_rounding x87_prec_80, x87_round_down
fld tword [rel .source_1]
fsin
fstp tword [rel .result_4]

; 64-bit mode, round-down
set_cw_precision_rounding x87_prec_64, x87_round_down
fld tword [rel .source_1]
fsin
fstp tword [rel .result_5]

; 32-bit mode, round-down
set_cw_precision_rounding x87_prec_32, x87_round_down
fld tword [rel .source_1]
fsin
fstp tword [rel .result_6]

; 80-bit mode, round-up
set_cw_precision_rounding x87_prec_80, x87_round_up
fld tword [rel .source_1]
fsin
fstp tword [rel .result_7]

; 64-bit mode, round-up
set_cw_precision_rounding x87_prec_64, x87_round_up
fld tword [rel .source_1]
fsin
fstp tword [rel .result_8]

; 32-bit mode, round-up
set_cw_precision_rounding x87_prec_32, x87_round_up
fld tword [rel .source_1]
fsin
fstp tword [rel .result_9]

; 80-bit mode, round-towards_zero
set_cw_precision_rounding x87_prec_80, x87_round_towards_zero
fld tword [rel .source_1]
fsin
fstp tword [rel .result_10]

; 64-bit mode, round-towards_zero
set_cw_precision_rounding x87_prec_64, x87_round_towards_zero
fld tword [rel .source_1]
fsin
fstp tword [rel .result_11]

; 32-bit mode, round-towards_zero
set_cw_precision_rounding x87_prec_32, x87_round_towards_zero
fld tword [rel .source_1]
fsin
fstp tword [rel .result_12]

; Fetch results
movups xmm0, [rel .result_1]
movups xmm1, [rel .result_2]
movups xmm2, [rel .result_3]
movups xmm3, [rel .result_4]
movups xmm4, [rel .result_5]
movups xmm5, [rel .result_6]
movups xmm6, [rel .result_7]
movups xmm7, [rel .result_8]
movups xmm8, [rel .result_9]
movups xmm9, [rel .result_10]
movups xmm10, [rel .result_11]
movups xmm11, [rel .result_12]

hlt

; Positive
.source_1:
dq 0x8222_2222_2222_2222
dw 0x3fff

.result_1:
dq 0
dq 0

.result_2:
dq 0
dq 0

.result_3:
dq 0
dq 0

.result_4:
dq 0
dq 0

.result_5:
dq 0
dq 0

.result_6:
dq 0
dq 0

.result_7:
dq 0
dq 0

.result_8:
dq 0
dq 0

.result_9:
dq 0
dq 0

.result_10:
dq 0
dq 0

.result_11:
dq 0
dq 0

.result_12:
dq 0
dq 0
