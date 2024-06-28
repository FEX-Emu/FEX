%ifdef CONFIG
{
  "RegData": {
    "RAX": "8"
  }
}
%endif

mov rdx, 0xe0000000

; This behaviour was seen around Wine 32-bit libraries
; Anything doing a call to a double application would spin
; the x87 stack on to the stack looking for fxam to return empty
; Empty in this case is that C0 and C3 is set while C2 is not

fninit
; Fill the x87 stack
fldz
fldz
fldz
fldz
fldz
fldz
fldz
fldz

mov eax, 0
mov ecx, 0

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
fstp qword [rdx + rcx * 8]
fwait
inc ecx
jmp .ExamineStack

.Done:

; Save how many we stored
mov eax, ecx

; Now fill with "Garbage"
fld1
fld1
fld1
fld1
fld1
fld1
fld1
fld1

.Reload:
; Now reload the stack
dec ecx
fld qword [rdx + rcx * 8]
cmp ecx, 0x0
jne .Reload;

hlt
