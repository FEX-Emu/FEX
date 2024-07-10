%ifdef CONFIG
{
  "RegData": {
    "R14": "0x0002000200024042",
    "R15": "0x0000040601030103"
  }
}
%endif

mov r15, 0
mov r14, 0

%macro test_value 2
  mov rax, 0
  fld tword [rel %2]
  fxam
  fnstsw ax
  ffreep
  and rax, (1 << 14) | (1 << 10) | (1 << 9) | (1 << 8)
  shr rax, 8

  shl %1, 8
  or %1, rax
%endmacro

fninit

test_value r14, .data_pos_invalid
test_value r14, .data_neg_invalid
test_value r14, .data_special_invalid
test_value r14, .data_special_neg_invalid
test_value r14, .data_unnormal
test_value r14, .data_neg_unnormal
test_value r14, .data_zero
test_value r14, .data_neg_zero

test_value r15, .data_normal
test_value r15, .data_neg_normal
test_value r15, .data_pos_snan
test_value r15, .data_neg_snan
test_value r15, .data_pos_qnan
test_value r15, .data_neg_qnan

hlt
align 32

.data_pos_invalid:
dq 0
dw 0x7fff

.data_neg_invalid:
dq 0
dw 0xffff

.data_special_invalid:
dq 1
dw 0x7fff

.data_special_neg_invalid:
dq 1
dw 0xffff

.data_unnormal:
dq 1
dw 0x0001

.data_neg_unnormal:
dq 1
dw 0x8001

.data_zero:
dq 0
dw 0x0000

.data_neg_zero:
dq 0
dw 0x8000

.data_pos_qnan:
dq (11b << 62) | (1)
dw 0x7FFF

.data_neg_qnan:
dq (11b << 62) | (1)
dw 0xFFFF

.data_pos_snan:
dq (10b << 62) | (1)
dw 0x7FFF

.data_neg_snan:
dq (10b << 62) | (1)
dw 0xFFFF

.data_normal:
dt 1.0

.data_neg_normal:
dt -1.0
