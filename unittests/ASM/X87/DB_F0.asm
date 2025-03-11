%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x0",
    "RCX": "0x1",
    "RDI": "0x40"
  }
}
%endif

lea rdx, [rel minus_one]
fld tword [rdx + 8 * 0]

lea rdx, [rel two]
fld tword [rdx + 8 * 0]

lea rdx, [rel one]
fld tword [rdx + 8 * 0]
fld tword [rdx + 8 * 0]

; Mask for CF, PF, ZF flags
mov rax, 0b1000101

; Stack:
; st(0) = 1.0
; st(1) = 1.0
; st(2) = 2.0
; st(3) = -1.0

; st(0) > st(i)
fcomi st3
pushfq
pop rbx
and rbx, rax

; st(0) < st(i)
fcomi st2
pushfq
pop rcx
and rcx, rax

; st(0) == st(i)
fcomi st1
pushfq
pop rdi
and rdi, rax

hlt

align 8
one:
  dt 1.0

align 8
two:
  dt 2.0

align 8
minus_one:
  dt -1.0
