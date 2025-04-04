%ifdef CONFIG
{
  "RegData": {
    "RAX":  "7",
    "RBX":  "0",
    "MM0":  "0x3ff0000000000000",
    "MM1":  "0x4070000000000000",
    "MM2":  "0x4060000000000000",
    "MM3":  "0x4050000000000000",
    "MM4":  "0x4040000000000000",
    "MM5":  "0x4030000000000000",
    "MM6":  "0x4020000000000000",
    "MM7":  "0x4000000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Set the stack with different values.
; Then do fincstp and store the stack values into MMX registers through memory
; such that MM0 has the value of ST0 and so on.

section .bss
align 8
temp: resq 1
stack: resq 8

section .text
global _start

_start:

mov rax, 0x3ff0000000000000 ; 1.0
mov [rel temp], rax
fld qword [rel temp]

mov rax, 0x4000000000000000 ; 2.0
mov [rel temp], rax
fld qword [rel temp]

mov rax, 0x4020000000000000 ; 4.0
mov [rel temp], rax
fld qword [rel temp]

mov rax, 0x4030000000000000
mov [rel temp], rax
fld qword [rel temp]

mov rax, 0x4040000000000000
mov [rel temp], rax
fld qword [rel temp]

mov rax, 0x4050000000000000
mov [rel temp], rax
fld qword [rel temp]

mov rax, 0x4060000000000000
mov [rel temp], rax
fld qword [rel temp]

mov rax, 0x4070000000000000
mov [rel temp], rax
fld qword [rel temp]

; Store top in RBX
xor rax, rax
xor rbx, rbx
fnstsw ax
shr ax, 11
and ax, 7
mov bx, ax

; Move the value of stop
; ST0 is currently 0x4070000000000000
fdecstp

; Store top in RAX
xor rax, rax
fnstsw ax
shr ax, 11
and ax, 7

; Now ST0 is 0x3ff0000000000000
fstp qword [rel stack + 8 * 0]
fstp qword [rel stack + 8 * 1]
fstp qword [rel stack + 8 * 2]
fstp qword [rel stack + 8 * 3]
fstp qword [rel stack + 8 * 4]
fstp qword [rel stack + 8 * 5]
fstp qword [rel stack + 8 * 6]
fstp qword [rel stack + 8 * 7]

movq mm0, [rel stack + 8 * 0]
movq mm1, [rel stack + 8 * 1]
movq mm2, [rel stack + 8 * 2]
movq mm3, [rel stack + 8 * 3]
movq mm4, [rel stack + 8 * 4]
movq mm5, [rel stack + 8 * 5]
movq mm6, [rel stack + 8 * 6]
movq mm7, [rel stack + 8 * 7]

hlt
