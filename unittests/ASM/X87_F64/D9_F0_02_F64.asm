%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3FDA827999FCEF32",
    "RBX": "0xBFE0000000000000",
    "RCX": "0x3FF0000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test F2XM1 with non-trivial inputs
; Test 1: 2^0.5 - 1 = sqrt(2) - 1 ~= 0.41421356237309515
; Test 2: 2^(-1) - 1 = -0.5
; Test 3: 2^1 - 1 = 1.0

mov rdx, 0xe0000000

; Test 1: f2xm1(0.5)
lea rsi, [rel data_half]
fld tword [rsi]
f2xm1
fstp qword [rdx]
mov rax, [rdx]

; Test 2: f2xm1(-1.0)
lea rsi, [rel data_neg1]
fld tword [rsi]
f2xm1
fstp qword [rdx]
mov rbx, [rdx]

; Test 3: f2xm1(1.0)
lea rsi, [rel data_pos1]
fld tword [rsi]
f2xm1
fstp qword [rdx]
mov rcx, [rdx]

hlt

align 8
data_half:
  dt 0.5
  dq 0

data_neg1:
  dt -1.0
  dq 0

data_pos1:
  dt 1.0
  dq 0
