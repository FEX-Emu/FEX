%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x80",
    "RCX": "0x00",
    "RDX": "0xC0",
    "RSI": "0x40",
    "R8":  "0x1",
    "R9":  "0x1",
    "R10": "0x0",
    "R11": "0x0"
  }
}
%endif

mov rbx, 0x01
mov rcx, 0x01
mov rdx, 0x80
mov rsi, 0x80

stc
rcr bl, 1
lahf
mov r8w, ax
shr r8, 8
and r8, 1 ; We only care about carry flag here

clc
rcr cl, 1
lahf
mov r9w, ax
shr r9, 8
and r9, 1 ; We only care about carry flag here

stc
rcr dl, 1
lahf
mov r10w, ax
shr r10, 8
and r10, 1 ; We only care about carry flag here

clc
rcr sil, 1
lahf
mov r11w, ax
shr r11, 8
and r11, 1 ; We only care about carry flag here

hlt
