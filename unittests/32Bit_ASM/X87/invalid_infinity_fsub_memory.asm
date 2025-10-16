%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test ∞ - ∞ using FSUB with memory operand = Invalid Operation (should set bit 0 of status word) - 32bit mode

; Setup memory with +infinity (0x7FF0000000000000 for double precision +infinity)
; In 32-bit mode, we need to store the double in two parts
mov dword [rel .data], 0x00000000    ; Low 32 bits
mov dword [rel .data+4], 0x7FF00000  ; High 32 bits

; Create +infinity by dividing 1.0 by 0.0
fld1
fldz
fdiv

; Subtract ∞ - ∞ using memory operand - this should be invalid
fsub qword [rel .data]

fstsw ax
and eax, 1

hlt

section .data
align 4096
.data: dq 0
