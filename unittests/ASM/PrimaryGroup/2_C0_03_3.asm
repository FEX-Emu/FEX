%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x40000000",
    "RCX": "0x00000000",
    "RDX": "0x60000000",
    "RSI": "0x20000000",
    "R8":  "0x1",
    "R9":  "0x1",
    "R10": "0x0",
    "R11": "0x0"
  }
}
%endif

mov rbx, 0x00000002
mov rcx, 0x00000002
mov rdx, 0x80000000
mov rsi, 0x80000000

stc
rcr ebx, 2
lahf
mov r8w, ax
shr r8, 8
and r8, 1 ; We only care about carry flag here

clc
rcr ecx, 2
lahf
mov r9w, ax
shr r9, 8
and r9, 1 ; We only care about carry flag here

stc
rcr edx, 2
lahf
mov r10w, ax
shr r10, 8
and r10, 1 ; We only care about carry flag here

clc
rcr esi, 2
lahf
mov r11w, ax
shr r11, 8
and r11, 1 ; We only care about carry flag here

hlt
