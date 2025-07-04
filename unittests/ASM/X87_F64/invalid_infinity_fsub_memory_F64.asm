%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

; Test ∞ - ∞ using FSUB with memory operand = Invalid Operation (should set bit 0 of status word) - reduced precision

; Create +infinity by dividing 1.0 by 0.0
fld1
fldz
fdiv

; Subtract ∞ - ∞ using memory operand - this should be invalid
lea rdx, [rel data]
fsub qword [rdx]

fstsw ax
and rax, 1

hlt

align 8
data:
  dq 0x7FF0000000000000  ; +infinity