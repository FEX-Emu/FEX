%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x40",
    "RDI": "0x00",
    "RDX": "0x60",
    "RSI": "0x20",
    "R8":  "0x1",
    "R9":  "0x1",
    "R10": "0x0",
    "R11": "0x0"
  }
}
%endif

mov rbx, 0x02
mov rdi, 0x02
mov rdx, 0x80
mov rsi, 0x80
mov rcx, 2

stc
rcr bl, cl
lahf
mov r8w, ax
shr r8, 8
and r8, 1 ; We only care about carry flag here

clc
rcr dil, cl
lahf
mov r9w, ax
shr r9, 8
and r9, 1 ; We only care about carry flag here

stc
rcr dl, cl
lahf
mov r10w, ax
shr r10, 8
and r10, 1 ; We only care about carry flag here

clc
rcr sil, cl
lahf
mov r11w, ax
shr r11, 8
and r11, 1 ; We only care about carry flag here

hlt
