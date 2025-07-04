%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test Invalid Operation with reduced precision (64-bit)
; Set precision control to 64-bit (PC = 10b)
fnstcw [.saved_cw]
mov ax, [.saved_cw]
and ax, 0xFCFF
or ax, 0x0200
mov [.new_cw], ax
fldcw [.new_cw]

; Perform invalid operation: 0.0 / 0.0
fldz
fldz
fdiv

fstsw ax
and eax, 1

; Restore original control word
fldcw [.saved_cw]

hlt

.saved_cw:  dw 0
.new_cw:    dw 0
