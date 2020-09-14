%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x00000006",
    "RCX": "0x00000004",
    "RDX": "0x00000002",
    "RSI": "0x00000000",
    "R8":  "0x0",
    "R9":  "0x0",
    "R10": "0x1",
    "R11": "0x1"
  }
}
%endif

mov rbx, 0x00000001
mov rcx, 0x00000001
mov rdx, 0x40000000
mov rsi, 0x40000000

stc
rcl ebx, 2
lahf
mov r8w, ax
shr r8, 8
and r8, 1 ; We only care about carry flag here

clc
rcl ecx, 2
lahf
mov r9w, ax
shr r9, 8
and r9, 1 ; We only care about carry flag here

stc
rcl edx, 2
lahf
mov r10w, ax
shr r10, 8
and r10, 1 ; We only care about carry flag here

clc
rcl esi, 2
lahf
mov r11w, ax
shr r11, 8
and r11, 1 ; We only care about carry flag here

hlt
