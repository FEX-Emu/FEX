%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x4008000000000000",
    "RBX":  "0x4008000000000000",
    "RCX":  "0x4024000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test fyl2x (y * log2(x)) with exact integer results in double precision.

mov rsi, 0xe0000000

; 1.0 * log2(8.0) = 3.0
lea rdx, [rel val_1_0]
fld tword [rdx]
lea rdx, [rel val_8_0]
fld tword [rdx]
fyl2x
fstp qword [rsi]
mov rax, [rsi]

; 3.0 * log2(2.0) = 3.0
lea rdx, [rel val_3_0]
fld tword [rdx]
lea rdx, [rel val_2_0]
fld tword [rdx]
fyl2x
fstp qword [rsi]
mov rbx, [rsi]

; 2.0 * log2(32.0) = 10.0
lea rdx, [rel val_2_0]
fld tword [rdx]
lea rdx, [rel val_32_0]
fld tword [rdx]
fyl2x
fstp qword [rsi]
mov rcx, [rsi]

hlt

align 8
val_1_0:
  dt 1.0
  dq 0
val_2_0:
  dt 2.0
  dq 0
val_3_0:
  dt 3.0
  dq 0
val_8_0:
  dt 8.0
  dq 0
val_32_0:
  dt 32.0
  dq 0
