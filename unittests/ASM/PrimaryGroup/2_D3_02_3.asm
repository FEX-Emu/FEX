%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x0000000000000006",
    "RDI": "0x0000000000000004",
    "RDX": "0x0000000000000002",
    "RSI": "0x0000000000000000",
    "R8":  "0x0",
    "R9":  "0x0",
    "R10": "0x1",
    "R11": "0x1"
  }
}
%endif

mov rbx, 0x0000000000000001
mov rdi, 0x0000000000000001
mov rdx, 0x4000000000000000
mov rsi, 0x4000000000000000
mov rcx, 2

stc
rcl rbx, cl
lahf
mov r8w, ax
shr r8, 8
and r8, 1 ; We only care about carry flag here

clc
rcl rdi, cl
lahf
mov r9w, ax
shr r9, 8
and r9, 1 ; We only care about carry flag here

stc
rcl rdx, cl
lahf
mov r10w, ax
shr r10, 8
and r10, 1 ; We only care about carry flag here

clc
rcl rsi, cl
lahf
mov r11w, ax
shr r11, 8
and r11, 1 ; We only care about carry flag here

hlt
