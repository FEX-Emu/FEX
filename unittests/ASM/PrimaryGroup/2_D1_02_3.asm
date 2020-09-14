%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x0000000000000003",
    "RCX": "0x0000000000000002",
    "RDX": "0x0000000000000001",
    "RSI": "0x0000000000000000",
    "R8":  "0x0",
    "R9":  "0x0",
    "R10": "0x1",
    "R11": "0x1"
  }
}
%endif

mov rbx, 0x0000000000000001
mov rcx, 0x0000000000000001
mov rdx, 0x8000000000000000
mov rsi, 0x8000000000000000

stc
rcl rbx, 1
lahf
mov r8w, ax
shr r8, 8
and r8, 1 ; We only care about carry flag here

clc
rcl rcx, 1
lahf
mov r9w, ax
shr r9, 8
and r9, 1 ; We only care about carry flag here

stc
rcl rdx, 1
lahf
mov r10w, ax
shr r10, 8
and r10, 1 ; We only care about carry flag here

clc
rcl rsi, 1
lahf
mov r11w, ax
shr r11, 8
and r11, 1 ; We only care about carry flag here

hlt

