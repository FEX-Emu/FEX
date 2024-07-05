%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x8000",
    "RBX": "0x8000",
    "RCX": "0x80000000",
    "RDX": "0x80000000",
    "RSI": "0x8000000000000000",
    "RDI": "0x8000000000000000"
  }
}
%endif

; FEX-Emu had a bug where x87 float to integer conversions weren't converting to the correct "integer indefinite" value for 16-bit conversions.
; Test 16-bit, 32-bit, and 64-bit to ensure correct "integer indefinite" results for all.
; The definition for "integer indefinite" is the smallest negative integer that can be represented.
; This is regardless of the input value being positive or negative.
fninit

; 16-bit
fld qword [rel .double_larger_than_int16]
fistp word [rel .data_res_pos_16]

fld qword [rel .double_smaller_than_int16]
fistp word [rel .data_res_neg_16]

; 32-bit
fld qword [rel .double_larger_than_int32]
fistp dword [rel .data_res_pos_32]

fld qword [rel .double_smaller_than_int32]
fistp dword [rel .data_res_neg_32]

; 64-bit
fld qword [rel .double_larger_than_int64]
fistp qword [rel .data_res_pos_64]

fld qword [rel .double_smaller_than_int64]
fistp qword [rel .data_res_neg_64]

; Load the results
movzx rax, word [rel .data_res_pos_16]
movzx rbx, word [rel .data_res_neg_16]

mov ecx, dword [rel .data_res_pos_32]
mov edx, dword [rel .data_res_neg_32]

mov rsi, qword [rel .data_res_pos_64]
mov rdi, qword [rel .data_res_neg_64]

hlt

; One-integer larger than what int16_t can hold
.double_larger_than_int16:
dq 32768.0
; One-integer smaller than what int16_t can hold
.double_smaller_than_int16:
dq -32769.0

; One-integer larger than what int32_t can hold
.double_larger_than_int32:
dq 2147483648.0
; One-integer smaller than what int32_t can hold
.double_smaller_than_int32:
dq -2147483649.0

; One-integer larger than what int64_t can hold
.double_larger_than_int64:
dq 9223372036854775808.0
; One-integer smaller than what int64_t can hold
.double_smaller_than_int64:
dq -9223372036854775809.0

.data_res_pos_16:
dw -1
.data_res_neg_16:
dw -1

.data_res_pos_32:
dd -1
.data_res_neg_32:
dd -1

.data_res_pos_64:
dq -1
.data_res_neg_64:
dq -1
