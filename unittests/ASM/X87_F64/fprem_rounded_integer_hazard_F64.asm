%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3FFAAAAAAAAAAAAA",
    "RBX": "0xBFFAAAAAAAAAAAAA"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Rounding hazard: x/y is mathematically just below 3, but fdiv rounds it up to 3.0
; exactly. frintz then yields q=3 with err==0, looking like an exact-quotient case.
; The fmsub(q, y, x) exactness check must catch the discrepancy and fall back so the
; libm-based handler returns the IEEE fmod result instead of the bogus inline result.
;
; y = 0x3FFAAAAAAAAAAAAB = 1.6666666666666667 (next double above 5/3)
; fmod(5.0, y) = 0x3FFAAAAAAAAAAAAA = 1.6666666666666665

mov rsi, 0xe0000000

; Test 1: fprem(5.0, 1.6666666666666667) = 1.6666666666666665
lea rdi, [rel data_y]
fld qword [rdi]
lea rdi, [rel data_x]
fld qword [rdi]
fprem
fstp qword [rsi]
mov rax, [rsi]
fstp st0

; Test 2: fprem(-5.0, 1.6666666666666667) = -1.6666666666666665
lea rdi, [rel data_y]
fld qword [rdi]
lea rdi, [rel data_neg_x]
fld qword [rdi]
fprem
fstp qword [rsi]
mov rbx, [rsi]
fstp st0

hlt

align 8
data_x:
  dq 0x4014000000000000     ; 5.0
data_neg_x:
  dq 0xC014000000000000     ; -5.0
data_y:
  dq 0x3FFAAAAAAAAAAAAB     ; 1.6666666666666667 (5/3 rounded up)
