%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0",
    "RBX":  "0"
  }
}
%endif

; FIST/FISTP to a 16-bit destination must not set IE when the input is a
; denormal that rounds into range. The overflow/NaN-Inf detection should
; only raise IE for exponent == 0x7fff or exponent >= 0x400e.

mov r15, 0xe0000000
fninit

mov dword [r15 + 0], 0x00000001
mov dword [r15 + 4], 0x00000000
mov word  [r15 + 8], 0x0000

fld tword [r15 + 0]

fistp word [r15 + 16]

movzx rax, word [r15 + 16]

fnstsw ax
movzx rbx, ax
and rbx, 1

xor rax, rax

hlt
