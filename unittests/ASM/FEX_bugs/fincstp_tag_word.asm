%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xff"
  }
}
%endif

mov word [rel .fxsave_data + 0], 0x037f ; FCW
mov byte [rel .fxsave_data + 4], 0xff   ; FTW

; FSW = 0
fxrstor [rel .fxsave_data]

fincstp

fxsave [rel .fxsave_data]
movzx rax, byte [rel .fxsave_data + 4] ; FTW

hlt

align 4096
.fxsave_data:
times 64 dq 0
