%ifdef CONFIG
{
  "RegData": {
    "RBX":  "0x0001"
  }
}
%endif

; FSCALE(+0, +Inf) is 0 * 2^(+Inf) = 0 * Inf, which must raise the invalid
; operation exception (IE=1).

mov r15, 0xe0000000
fninit

mov dword [r15 + 0], 0x00000000
mov dword [r15 + 4], 0x7ff00000

fld qword [r15 + 0]
fldz

fscale

fstp qword [r15 + 16]
fstp st0

fnstsw ax
movzx rbx, ax
and rbx, 1

hlt
