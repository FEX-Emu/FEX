%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x40",
    "RCX": "0x00",
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
mov rcx, 0x02
mov rdx, 0x80
mov rsi, 0x80

stc
rcr bl, 2
lahf
mov r8w, ax
shr r8, 8
and r8, 1 ; We only care about carry flag here

clc
rcr cl, 2
lahf
mov r9w, ax
shr r9, 8
and r9, 1 ; We only care about carry flag here

stc
rcr dl, 2
lahf
mov r10w, ax
shr r10, 8
and r10, 1 ; We only care about carry flag here

clc
rcr sil, 2
lahf
mov r11w, ax
shr r11, 8
and r11, 1 ; We only care about carry flag here

hlt
