;; Simpler versions of FXAM_Push* tests.
%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x6",
    "RBX": "0x0400",
    "RCX": "0x4000",
    "RDX": "0x4100",
    "R8":  "0x4200440044000700",
    "R9":  "0x0100030000000000",
    "R10": "0x0000000004000400",
    "R11": "0x0000050006004400"
  }
}
%endif

mov rdx, 0xe0000000

fninit
;; Before adding anything to the stack, lets examine it.
;; The result should be empty.
fxam
fwait

fnstsw ax 
and ax, 0x4500 ; should be 0x4100 for empty
mov edx, eax

;; Get the C0 - C3 flags and
;; pack up to 4 results into a GPR
%macro examine_packed 2
  fld tword [rel %2]
  fxam
  fwait
  fnstsw ax
  and eax, 0x4700
  shl %1, 16
  or %1, rax
  fstp st0
%endmacro

xor r8d, r8d
examine_packed r8, .data_neg_zero
examine_packed r8, .data_denormal
examine_packed r8, .data_pseudo_denormal
examine_packed r8, .data_neg_infinity

xor r9d, r9d
examine_packed r9, .data_qnan
examine_packed r9, .data_neg_snan
examine_packed r9, .data_unnormal
examine_packed r9, .data_pseudo_infinity

xor r10d, r10d
examine_packed r10, .data_pseudo_nan
examine_packed r10, .data_pseudo_zero
examine_packed r10, .data_min_exponent
examine_packed r10, .data_max_exponent

xor r11d, r11d
examine_packed r11, .data_infinity
examine_packed r11, .data_neg_one
examine_packed r11, .data_pseudo_denormal_fraction

fldz
fxam 
fwait 

fnstsw ax
and ax, 0x4500 ; should be 0x4000 for zero
mov ecx, eax

fld1
fxam
fwait

fnstsw ax
mov ebx, eax
and ebx, 0x4500 ; should be 0x0400 for normal

;; Top should be 6
;; right shift status word by 11 and and with 0x7.
shr eax, 11
and eax, 0x7


hlt

align 16
.data_neg_zero:
dq 0
dw 0x8000

.data_denormal:
dq 1
dw 0x0000

.data_pseudo_denormal:
dq (1 << 63)
dw 0x0000

.data_neg_infinity:
dq (1 << 63)
dw 0xFFFF

.data_qnan:
dq (11b << 62)
dw 0x7FFF

.data_neg_snan:
dq (10b << 62) | 1
dw 0xFFFF

.data_unnormal:
dq (1 << 62)
dw 0x3FFF

.data_pseudo_infinity:
dq 0
dw 0x7FFF

.data_pseudo_nan:
dq (1 << 62)
dw 0x7FFF

.data_pseudo_zero:
dq 0
dw 0x3FFF

.data_min_exponent:
dq (1 << 63)
dw 0x0001

.data_max_exponent:
dq (1 << 63)
dw 0x7FFE

.data_infinity:
dq (1 << 63)
dw 0x7FFF

.data_neg_one:
dq (1 << 63)
dw 0xBFFF

.data_pseudo_denormal_fraction:
dq (1 << 63) | 1
dw 0x0000
