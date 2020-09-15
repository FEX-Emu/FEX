%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x8000000000000000",
    "RCX": "0x0000000000000000",
    "RDX": "0xC000000000000000",
    "RSI": "0x4000000000000000",
    "R8":  "0x1",
    "R9":  "0x1",
    "R10": "0x0",
    "R11": "0x0"
  }
}
%endif

mov rbx, 0x0000000000000001
mov rcx, 0x0000000000000001
mov rdx, 0x8000000000000000
mov rsi, 0x8000000000000000

stc
rcr rbx, 1
lahf
mov r8w, ax
shr r8, 8
and r8, 1 ; We only care about carry flag here

clc
rcr rcx, 1
lahf
mov r9w, ax
shr r9, 8
and r9, 1 ; We only care about carry flag here

stc
rcr rdx, 1
lahf
mov r10w, ax
shr r10, 8
and r10, 1 ; We only care about carry flag here

clc
rcr rsi, 1
lahf
mov r11w, ax
shr r11, 8
and r11, 1 ; We only care about carry flag here

hlt
