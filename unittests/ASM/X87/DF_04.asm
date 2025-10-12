%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x0506070801020304", "0x0000000000000012"],
    "XMM1":  ["0x6576879821324354", "0x0000000000000000"],
    "XMM2":  ["0xB90984060D355548", "0x000000000000C03B"],
    "XMM3":  ["0xA83732340C01F070", "0x000000000000C03B"],
    "XMM4":  ["0xFFAA6DA43613FED0", "0x000000000000C03A"],
    "XMM5":  ["0x0000000000000001", "0x0000000000000000"],
    "XMM6":  ["0x0000000000000001", "0x0000000000008000"],
    "XMM7":  ["0x0000000000000000", "0x0000000000008000"],
    "XMM8":  ["0x0000000000000000", "0x0000000000008000"],
    "XMM9":  ["0x0000000000000000", "0x0000000000000000"],
    "XMM10":  ["0x0000000000000001", "0x0000000000008000"],
    "XMM11":  ["0x0000000000000001", "0x0000000000000000"]
  }
}
%endif

fbld [rel .data_0]
fbstp [rel .res_data_0]
movups xmm0, [rel .res_data_0]

fbld [rel .data_1]
fbstp [rel .res_data_1]
movups xmm1, [rel .res_data_1]

; Check encoding of invalid BCD
fbld [rel .data_2]
fstp tword [rel .res_data_2]
movups xmm2, [rel .res_data_2]

fbld [rel .data_3]
fstp tword [rel .res_data_3]
movups xmm3, [rel .res_data_3]

fbld [rel .data_4]
fstp tword [rel .res_data_4]
movups xmm4, [rel .res_data_4]

; Some special values
fld tword [rel .data_5]
fbstp [rel .res_data_5]
movups xmm5, [rel .res_data_5]

fld tword [rel .data_6]
fbstp [rel .res_data_6]
movups xmm6, [rel .res_data_6]

fld tword [rel .data_7]
fbstp [rel .res_data_7]
movups xmm7, [rel .res_data_7]

; Values that choose +- 0 or +-1 depending on rounding mode
; -1 < F < -0
; +0 < F < +1
fld tword [rel .data_8]
fbstp [rel .res_data_8]
movups xmm8, [rel .res_data_8]

fld tword [rel .data_9]
fbstp [rel .res_data_9]
movups xmm9, [rel .res_data_9]

; Swap control word
fnstcw [rel .cw]
mov ax, [rel .cw]
and ax, ~(3 << 10)
or eax, 1 << 10 ; Round down
mov [rel .cw], ax
fldcw [rel .cw]

fld tword [rel .data_10]
fbstp [rel .res_data_10]
movups xmm10, [rel .res_data_10]

; Swap control word
fnstcw [rel .cw]
mov ax, [rel .cw]
and ax, ~(3 << 10)
or eax, 2 << 10 ; Round up
mov [rel .cw], ax
fldcw [rel .cw]

fld tword [rel .data_11]
fbstp [rel .res_data_11]
movups xmm11, [rel .res_data_11]

; Values that generate Invalicating floating point operation exception
; -inf
; +inf
; Negative value too large for destination format
; Positive value too large for destination format
; NaN
; On IA the indefinite BCD result is still stored to memory

; XXX: We don't support IA on this

hlt

align 4096
.cw:
dw 0

.data_0:
dd 0x01020304
dd 0x05060708
dd 0x09101112
dd 0x13141516
.data_1:
dd 0x21324354
dd 0x65768798
dd 0x00000000
dd 0x00000000
.data_2:
dd 0xFFFFFFFF
dd 0xFFFFFFFF
dd 0xFFFFFFFF
dd 0xFFFFFFFF
.data_3:
dd 0xF0F0F0F0
dd 0xF0F0F0F0
dd 0xF0F0F0F0
dd 0xF0F0F0F0
.data_4:
dd 0x0A0B0C0D
dd 0x0E0FAAAB
dd 0xACADAEAF
dd 0xBABBBCBD
.data_5:
dt 1.0
.data_6:
dt -1.0
.data_7:
dt -0.0
.data_8:
dt -0.5
.data_9:
dt 0.5
.data_10:
dt -0.5
.data_11:
dt 0.5

.res_data_0:
dq 0
dq 0
.res_data_1:
dq 0
dq 0
.res_data_2:
dq 0
dq 0
.res_data_3:
dq 0
dq 0
.res_data_4:
dq 0
dq 0
.res_data_5:
dq 0
dq 0
.res_data_6:
dq 0
dq 0
.res_data_7:
dq 0
dq 0
.res_data_8:
dq 0
dq 0
.res_data_9:
dq 0
dq 0
.res_data_10:
dq 0
dq 0
.res_data_11:
dq 0
dq 0

