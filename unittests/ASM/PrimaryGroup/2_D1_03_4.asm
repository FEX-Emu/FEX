%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x8000",
    "RCX": "0x0000",
    "RDX": "0xC000",
    "RSI": "0x4000",
    "R8":  "0x1",
    "R9":  "0x0",
    "R10": "0x0",
    "R11": "0x1"
  }
}
%endif

mov rbx, 0x0001
mov rcx, 0x0001
mov rdx, 0x8000
mov rsi, 0x8000
mov r15, 1

stc
rcr bx, 1
mov r8, 0
cmovo r8, r15 ; We only care about OF here

clc
rcr cx, 1
mov r9, 0
cmovo r9, r15 ; We only care about OF here

stc
rcr dx, 1
mov r10, 0
cmovo r10, r15 ; We only care about OF here

clc
rcr si, 1
mov r11, 0
cmovo r11, r15 ; We only care about OF here

hlt
