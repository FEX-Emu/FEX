%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1"
  }
}
%endif


fxrstor [rel .fxsave_data]
fst st0

fxsave [rel .save]
movzx eax, byte [rel .save + 4]

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
