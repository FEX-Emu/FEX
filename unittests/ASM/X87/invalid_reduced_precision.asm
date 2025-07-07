%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test Invalid Operation with reduced precision (64-bit)
; Set precision control to 64-bit (PC = 10b)
fnstcw [rel saved_cw]
mov ax, [rel saved_cw]
and ax, 0xFCFF
or ax, 0x0200
mov [rel new_cw], ax
fldcw [rel new_cw]

; Perform invalid operation: 0.0 / 0.0
fldz
fldz
fdiv

fstsw ax
and rax, 1

; Restore original control word
fldcw [rel saved_cw]

hlt

align 8
saved_cw:  dw 0
new_cw:    dw 0
