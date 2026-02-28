%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000000000002",
    "RBX": "0x0000000000000001",
    "RCX": "0x0000000000000002",
    "RDX": "0x0000000000000001",
    "RSI": "0xFFFFFFFFFFFFFFFE",
    "RDI": "0xFFFFFFFFFFFFFFFF",
    "R8":  "0xFFFFFFFFFFFFFFFF",
    "R9":  "0xFFFFFFFFFFFFFFFF"
  }
}
%endif

%include "x87cw.mac"

mov rsp, 0xe000_1000

; FISTP m64int rounding mode tests.
; Tests all 4 rounding modes with 64-bit integer store.
; Existing Rounding.asm only covers fist(p) with 16/32-bit destinations;
; this test covers the 64-bit (m64int) case.
;
; Value under test: 1.5
;   RC=00 nearest-even  -> 2  (rounds to even)
;   RC=01 round-down    -> 1  (floor)
;   RC=10 round-up      -> 2  (ceil)
;   RC=11 toward-zero   -> 1  (truncate)
;
; Value under test: -1.5
;   RC=00 nearest-even  -> -2 (rounds to even)
;   RC=01 round-down    -> -2 (floor toward -inf)
;   RC=10 round-up      -> -1 (ceil toward +inf)
;   RC=11 toward-zero   -> -1 (truncate toward 0)

; --- Positive 1.5 ---

; Round to nearest (RC=00): 1.5 -> 2
finit
set_cw_precision_rounding x87_prec_80, x87_round_nearest
fld qword [rel val_1_5]
fistp qword [rel tmp]
mov rax, [rel tmp]

; Round down (RC=01): 1.5 -> 1
finit
set_cw_precision_rounding x87_prec_80, x87_round_down
fld qword [rel val_1_5]
fistp qword [rel tmp]
mov rbx, [rel tmp]

; Round up (RC=10): 1.5 -> 2
finit
set_cw_precision_rounding x87_prec_80, x87_round_up
fld qword [rel val_1_5]
fistp qword [rel tmp]
mov rcx, [rel tmp]

; Round toward zero (RC=11): 1.5 -> 1
finit
set_cw_precision_rounding x87_prec_80, x87_round_towards_zero
fld qword [rel val_1_5]
fistp qword [rel tmp]
mov rdx, [rel tmp]

; --- Negative -1.5 ---

; Round to nearest (RC=00): -1.5 -> -2
finit
set_cw_precision_rounding x87_prec_80, x87_round_nearest
fld qword [rel val_n1_5]
fistp qword [rel tmp]
mov rsi, [rel tmp]

; Round down (RC=01): -1.5 -> -2
finit
set_cw_precision_rounding x87_prec_80, x87_round_down
fld qword [rel val_n1_5]
fistp qword [rel tmp]
mov rdi, [rel tmp]

; Round up (RC=10): -1.5 -> -1
finit
set_cw_precision_rounding x87_prec_80, x87_round_up
fld qword [rel val_n1_5]
fistp qword [rel tmp]
mov r8, [rel tmp]

; Round toward zero (RC=11): -1.5 -> -1
finit
set_cw_precision_rounding x87_prec_80, x87_round_towards_zero
fld qword [rel val_n1_5]
fistp qword [rel tmp]
mov r9, [rel tmp]

hlt

align 8
val_1_5:  dq 1.5
val_n1_5: dq -1.5
tmp:      dq 0
