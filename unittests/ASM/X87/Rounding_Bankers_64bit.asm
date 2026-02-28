%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000000000000",
    "RBX": "0x0000000000000002",
    "RCX": "0x0000000000000002",
    "RDX": "0x0000000000000004",
    "RSI": "0xFFFFFFFFFFFFFFFE",
    "RDI": "0xFFFFFFFFFFFFFFFE"
  }
}
%endif

; FISTP m64int banker's rounding (round-to-nearest-even) edge cases.
; Default x87 rounding mode (RC=00 in FCW).
; At exact midpoints, the result rounds to the nearest EVEN integer.
;
; Related: issue #779

; 0.5 -> 0 (nearest even)
finit
fld qword [rel val_0_5]
fistp qword [rel tmp]
mov rax, [rel tmp]

; 1.5 -> 2 (nearest even)
finit
fld qword [rel val_1_5]
fistp qword [rel tmp]
mov rbx, [rel tmp]

; 2.5 -> 2 (nearest even)
finit
fld qword [rel val_2_5]
fistp qword [rel tmp]
mov rcx, [rel tmp]

; 3.5 -> 4 (nearest even)
finit
fld qword [rel val_3_5]
fistp qword [rel tmp]
mov rdx, [rel tmp]

; -1.5 -> -2 (nearest even)
finit
fld qword [rel val_n1_5]
fistp qword [rel tmp]
mov rsi, [rel tmp]

; -2.5 -> -2 (nearest even)
finit
fld qword [rel val_n2_5]
fistp qword [rel tmp]
mov rdi, [rel tmp]

hlt

align 8
val_0_5:  dq 0.5
val_1_5:  dq 1.5
val_2_5:  dq 2.5
val_3_5:  dq 3.5
val_n1_5: dq -1.5
val_n2_5: dq -2.5
tmp:      dq 0
