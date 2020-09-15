%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x0003",
    "RCX": "0x0002",
    "RDX": "0x0001",
    "RSI": "0x0000",
    "R8":  "0x0",
    "R9":  "0x0",
    "R10": "0x1",
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
rcl bx, 1
mov r8, 0
cmovo r8, r15 ; We only care about OF here

clc
rcl cx, 1
mov r9, 0
cmovo r9, r15 ; We only care about OF here

stc
rcl dx, 1
mov r10, 0
cmovo r10, r15 ; We only care about OF here

clc
rcl si, 1
mov r11, 0
cmovo r11, r15 ; We only care about OF here

hlt
