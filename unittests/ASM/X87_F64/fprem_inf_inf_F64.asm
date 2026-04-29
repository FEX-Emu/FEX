%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "RBX": "1",
    "RCX": "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; fprem(+inf, +inf) = NaN, fprem(-inf, +inf) = NaN, fprem1(+inf, +inf) = NaN
mov rdx, 0xe0000000

; Test 1: fprem(+inf, +inf) should be NaN
lea rsi, [rel data_pos_inf]
fld tword [rsi]
fld tword [rsi]
fprem
fstp qword [rdx]
fstp st0
movsd xmm0, [rdx]
ucomisd xmm0, xmm0
setp al
movzx rax, al

; Test 2: fprem(-inf, +inf) should be NaN
lea rsi, [rel data_pos_inf]
fld tword [rsi]
lea rsi, [rel data_neg_inf]
fld tword [rsi]
fprem
fstp qword [rdx]
fstp st0
movsd xmm0, [rdx]
ucomisd xmm0, xmm0
setp bl
movzx rbx, bl

; Test 3: fprem1(+inf, +inf) should be NaN
lea rsi, [rel data_pos_inf]
fld tword [rsi]
fld tword [rsi]
fprem1
fstp qword [rdx]
fstp st0
movsd xmm0, [rdx]
ucomisd xmm0, xmm0
setp cl
movzx rcx, cl

hlt

align 8
data_pos_inf:
  dq 0x8000000000000000
  dw 0x7FFF
  dq 0
data_neg_inf:
  dq 0x8000000000000000
  dw 0xFFFF
  dq 0
