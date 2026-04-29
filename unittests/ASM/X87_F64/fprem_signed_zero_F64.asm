%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000000000000",
    "RBX": "0x8000000000000000",
    "RCX": "0x0000000000000000",
    "RDX": "0x8000000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rsi, 0xe0000000

; Test 1: fprem(6.0, 3.0) = +0.0
lea rdi, [rel data_3_0]
fld tword [rdi]
lea rdi, [rel data_6_0]
fld tword [rdi]
fprem
fstp qword [rsi]
mov rax, [rsi]
fstp st0

; Test 2: fprem(-6.0, 3.0) = -0.0
lea rdi, [rel data_3_0]
fld tword [rdi]
lea rdi, [rel data_neg6_0]
fld tword [rdi]
fprem
fstp qword [rsi]
mov rbx, [rsi]
fstp st0

; Test 3: fprem1(6.0, 3.0) = +0.0
lea rdi, [rel data_3_0]
fld tword [rdi]
lea rdi, [rel data_6_0]
fld tword [rdi]
fprem1
fstp qword [rsi]
mov rcx, [rsi]
fstp st0

; Test 4: fprem1(-6.0, 3.0) = -0.0
lea rdi, [rel data_3_0]
fld tword [rdi]
lea rdi, [rel data_neg6_0]
fld tword [rdi]
fprem1
fstp qword [rsi]
mov rdx, [rsi]
fstp st0

hlt

align 8
data_6_0:
  dt 6.0
  dq 0
data_neg6_0:
  dt -6.0
  dq 0
data_3_0:
  dt 3.0
  dq 0
