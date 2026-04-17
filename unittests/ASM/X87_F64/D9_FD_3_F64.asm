%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4098000000000000",
    "RBX": "0x3FF8000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test FSCALE with double-precision verification
; Test 1: 1.5 * 2^10 = 1536.0 (0x4098000000000000)
; Test 2: 3.0 * 2^(-1) = 1.5  (0x3FF8000000000000)

mov rcx, 0xe0000000

; Test 1: 1.5 * 2^10
lea rdx, [rel data_exp1]
fld tword [rdx]
lea rdx, [rel data_base1]
fld tword [rdx]
fscale
fstp qword [rcx]
mov rax, [rcx]
fstp st0

; Test 2: 3.0 * 2^(-1)
lea rdx, [rel data_exp2]
fld tword [rdx]
lea rdx, [rel data_base2]
fld tword [rdx]
fscale
fstp qword [rcx]
mov rbx, [rcx]
fstp st0

hlt

align 8
data_base1:
  dt 1.5
  dq 0

data_exp1:
  dt 10.0
  dq 0

data_base2:
  dt 3.0
  dq 0

data_exp2:
  dt -1.0
  dq 0
