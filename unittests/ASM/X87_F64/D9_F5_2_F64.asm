%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3FF8000000000000",
    "RBX": "0xBFF0000000000000",
    "RCX": "0x3FF0000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

; Test 1: remainder(7.5, 3.0) = 1.5  (q = round(2.5) = 2)
lea rsi, [rel data_3_0]
fld tword [rsi]
lea rsi, [rel data_7_5]
fld tword [rsi]
fprem1
fstp qword [rdx]
mov rax, [rdx]
fstp st0

; Test 2: remainder(5.0, 3.0) = -1.0  (q = round(1.667) = 2)
lea rsi, [rel data_3_0]
fld tword [rsi]
lea rsi, [rel data_5_0]
fld tword [rsi]
fprem1
fstp qword [rdx]
mov rbx, [rdx]
fstp st0

; Test 3: remainder(10.0, 3.0) = 1.0  (q = round(3.333) = 3)
lea rsi, [rel data_3_0]
fld tword [rsi]
lea rsi, [rel data_10_0]
fld tword [rsi]
fprem1
fstp qword [rdx]
mov rcx, [rdx]
fstp st0

hlt

align 8
data_7_5:
  dt 7.5
  dq 0
data_5_0:
  dt 5.0
  dq 0
data_10_0:
  dt 10.0
  dq 0
data_3_0:
  dt 3.0
  dq 0
