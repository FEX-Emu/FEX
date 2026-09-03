%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3",
    "RBX": "0x81"
  }
}
%endif

; FXCH: empty and empty operands -> mark both valid
fxrstor [rel .fxsave_data]
fxch st0, st1

fxsave [rel .save]
movzx eax, byte [rel .save + 4]

; FXCH: valid and empty operands -> mark both valid
finit
fldz
fxch st0, st1

fxsave [rel .save2]
movzx ebx, byte [rel .save2 + 4]

hlt

align 16
.fxsave_data:
dw 0x037f       ; FCW
dw 0x0000       ; FSW
dw 0x0000       ; FTW
times 506 db 0

align 16
.save:
times 64 dq 0

align 16
.save2:
times 64 dq 0
