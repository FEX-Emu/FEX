%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000000000000",
    "RBX": "0x0000000000000000",
    "RCX": "0x0000000000000000",
    "RDX": "0x8000000000000000",
    "R8":  "0x8000000000000000",
    "R9":  "0x0000000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Exact-quotient inputs: x/y is mathematically an exact integer, so frintz(fdiv) leaves
; the inline path's err == 0. Verifies that fmsub(q, y, x) exactness check lets the fast
; path produce sign(x) * 0 instead of falling through to the libm fallback.

mov rsi, 0xe0000000

; Test 1: fprem(1.0, 1.0) = +0.0
lea rdi, [rel data_1_0]
fld tword [rdi]
lea rdi, [rel data_1_0]
fld tword [rdi]
fprem
fstp qword [rsi]
mov rax, [rsi]
fstp st0

; Test 2: fprem(8.0, 2.0) = +0.0
lea rdi, [rel data_2_0]
fld tword [rdi]
lea rdi, [rel data_8_0]
fld tword [rdi]
fprem
fstp qword [rsi]
mov rbx, [rsi]
fstp st0

; Test 3: fprem(10.0, 5.0) = +0.0
lea rdi, [rel data_5_0]
fld tword [rdi]
lea rdi, [rel data_10_0]
fld tword [rdi]
fprem
fstp qword [rsi]
mov rcx, [rsi]
fstp st0

; Test 4: fprem(-5.0, 5.0) = -0.0
lea rdi, [rel data_5_0]
fld tword [rdi]
lea rdi, [rel data_neg5_0]
fld tword [rdi]
fprem
fstp qword [rsi]
mov rdx, [rsi]
fstp st0

; Test 5: fprem(-1.0, 1.0) = -0.0
lea rdi, [rel data_1_0]
fld tword [rdi]
lea rdi, [rel data_neg1_0]
fld tword [rdi]
fprem
fstp qword [rsi]
mov r8, [rsi]
fstp st0

; Test 6: fprem(0.5, 0.5) = +0.0  (smaller magnitude exact quotient)
lea rdi, [rel data_0_5]
fld tword [rdi]
lea rdi, [rel data_0_5]
fld tword [rdi]
fprem
fstp qword [rsi]
mov r9, [rsi]
fstp st0

hlt

align 8
data_1_0:
  dt 1.0
  dq 0
data_neg1_0:
  dt -1.0
  dq 0
data_2_0:
  dt 2.0
  dq 0
data_5_0:
  dt 5.0
  dq 0
data_neg5_0:
  dt -5.0
  dq 0
data_8_0:
  dt 8.0
  dq 0
data_10_0:
  dt 10.0
  dq 0
data_0_5:
  dt 0.5
  dq 0
