%ifdef CONFIG
{
  "RegData": {
    "RAX": "8"
  }
}
%endif

fninit
fld1
fld1
fld1
fld1
fld1
fld1
fld1
fld1

mov ebx, 0

.ExamineStack:
; Examine st(0)
fxam
fwait
; Get the results in to AX
fnstsw ax
and ax, 0x4500
; Check for empty
cmp ax, 0x4100
je .Done

; Now push the x87 stack value
; We know it isn't empty
fstp st0
fwait
inc ebx
jmp .ExamineStack
.Done:
mov eax, ebx
hlt
