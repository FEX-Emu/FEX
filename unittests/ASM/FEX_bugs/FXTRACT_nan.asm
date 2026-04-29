%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x7ff8000000000000",
    "RBX":  "0x7ff8000000000000"
  }
}
%endif

; FXTRACT(QNaN) must propagate the NaN into both the exponent and significand
; slots.

mov r15, 0xe0000000
fninit

mov dword [r15 + 0], 0x00000000
mov dword [r15 + 4], 0x7ff80000
fld qword [r15 + 0]

fxtract

fstp qword [r15 + 16]
fstp qword [r15 + 24]

mov rax, [r15 + 24]
mov rbx, [r15 + 16]

hlt
