%ifdef CONFIG
{
  "RegData": {
    "R8": "0",
    "R9": "0",
    "R10": "0",
    "R11": "0",
    "R12": "0",
    "R13": "0x8000000000000000"
  }
}
%endif

; scale by zero (st1 == 0)
mov rax, 0
mov qword [rel intstor], rax
finit
fild qword [rel intstor]
fldz
fscale
fst qword [rel intstor]
mov r8, [rel intstor]

; scale by zero (st1 == 1)
mov rax, 1
mov qword [rel intstor], rax
finit
fild qword [rel intstor]
fldz
fscale
fst qword [rel intstor]
mov r9, [rel intstor]

; scale by zero (st1 == 100)
mov rax, 100
mov qword [rel intstor], rax
finit
fild qword [rel intstor]
fldz
fscale
fst qword [rel intstor]
mov r10, [rel intstor]

; scale by zero (st1 == 1024)
mov rax, 1024
mov qword [rel intstor], rax
finit
fild qword [rel intstor]
fldz
fscale
fst qword [rel intstor]
mov r11, [rel intstor]

; scale by zero (st1 == 1048576)
mov rax, 1048576
mov qword [rel intstor], rax
finit
fild qword [rel intstor]
fldz
fscale
fst qword [rel intstor]
mov r12, [rel intstor]

; tests scaling negative zero
mov rax, 1048576
mov qword [rel intstor], rax
finit
fild qword [rel intstor]
fld qword [rel neg_zero]
fscale
fst qword [rel intstor]
mov r13, [rel intstor]

hlt

align 4096
neg_zero: dq 0x8000000000000000   ; -0.0

intstor: dq 0
