%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x03",
    "RCX": "0x02",
    "RDX": "0x81",
    "RSI": "0x00",
    "R8":  "0x0",
    "R9":  "0x0",
    "R10": "0x1",
    "R11": "0x1"
  }
}
%endif

mov rbx, 0x01
mov rcx, 0x01
mov rdx, 0x40
mov rsi, 0x80
mov r15, 1

stc
rcl bl, 1
mov r8, 0
cmovo r8, r15 ; We only care about OF here

clc
rcl cl, 1
mov r9, 0
cmovo r9, r15 ; We only care about OF here

stc
rcl dl, 1
mov r10, 0
cmovo r10, r15 ; We only care about OF here

clc
rcl sil, 1
mov r11, 0
cmovo r11, r15 ; We only care about OF here

hlt
