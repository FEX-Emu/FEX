%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4014000000000000",
    "RBX": "0xC014000000000000",
    "RCX": "0x4014000000000000",
    "RDX": "0xC014000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; fprem1(finite, inf) should return ST(0) unchanged (Intel spec)
mov rsi, 0xe0000000

; Test 1: fprem1(5.0, +inf) = 5.0
lea rdi, [rel data_pos_inf]
fld tword [rdi]
lea rdi, [rel data_5_0]
fld tword [rdi]
fprem1
fstp qword [rsi]
mov rax, [rsi]
fstp st0

; Test 2: fprem1(-5.0, +inf) = -5.0
lea rdi, [rel data_pos_inf]
fld tword [rdi]
lea rdi, [rel data_neg5_0]
fld tword [rdi]
fprem1
fstp qword [rsi]
mov rbx, [rsi]
fstp st0

; Test 3: fprem1(5.0, -inf) = 5.0
lea rdi, [rel data_neg_inf]
fld tword [rdi]
lea rdi, [rel data_5_0]
fld tword [rdi]
fprem1
fstp qword [rsi]
mov rcx, [rsi]
fstp st0

; Test 4: fprem1(-5.0, -inf) = -5.0
lea rdi, [rel data_neg_inf]
fld tword [rdi]
lea rdi, [rel data_neg5_0]
fld tword [rdi]
fprem1
fstp qword [rsi]
mov rdx, [rsi]
fstp st0

hlt

align 8
data_5_0:
  dt 5.0
  dq 0
data_neg5_0:
  dt -5.0
  dq 0
data_pos_inf:
  dq 0x8000000000000000
  dw 0x7FFF
  dq 0
data_neg_inf:
  dq 0x8000000000000000
  dw 0xFFFF
  dq 0
