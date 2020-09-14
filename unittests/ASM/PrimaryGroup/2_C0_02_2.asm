%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x0006",
    "RCX": "0x0004",
    "RDX": "0x0002",
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
mov rdx, 0x4000
mov rsi, 0x4000

stc
rcl bx, 2
lahf
mov r8w, ax
shr r8, 8
and r8, 1 ; We only care about carry flag here

clc
rcl cx, 2
lahf
mov r9w, ax
shr r9, 8
and r9, 1 ; We only care about carry flag here

stc
rcl dx, 2
lahf
mov r10w, ax
shr r10, 8
and r10, 1 ; We only care about carry flag here

clc
rcl si, 2
lahf
mov r11w, ax
shr r11, 8
and r11, 1 ; We only care about carry flag here

hlt
