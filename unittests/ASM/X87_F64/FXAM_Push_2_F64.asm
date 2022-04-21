%ifdef CONFIG
{
  "RegData": {
    "RCX": "0"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

; This behaviour was seen around Wine 32-bit libraries
; Anything doing a call to a double application would spin
; the x87 stack on to the stack looking for fxam to return empty
; Empty in this case is that C0 and C3 is set whiel C2 is not

fninit
; Empty stack to make sure we don't push anything

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

hlt
