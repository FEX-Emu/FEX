%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x80",
    "RCX": "0x00",
    "RDX": "0xC0",
    "RSI": "0x40",
    "R8":  "0x1",
    "R9":  "0x0",
    "R10": "0x0",
    "R11": "0x1"
  }
}
%endif

mov rbx, 0x01
mov rcx, 0x01
mov rdx, 0x80
mov rsi, 0x80
mov r15, 1

stc
rcr bl, 1
mov r8, 0
cmovo r8, r15 ; We only care about OF here

clc
rcr cl, 1
mov r9, 0
cmovo r9, r15 ; We only care about OF here

stc
rcr dl, 1
mov r10, 0
cmovo r10, r15 ; We only care about OF here

clc
rcr sil, 1
mov r11, 0
cmovo r11, r15 ; We only care about OF here

hlt
