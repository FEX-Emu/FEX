%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x06",
    "RCX": "0x04",
    "RDX": "0x02",
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
mov rsi, 0x40

stc
rcl bl, 2
lahf
mov r8w, ax
shr r8, 8
and r8, 1 ; We only care about carry flag here

clc
rcl cl, 2
lahf
mov r9w, ax
shr r9, 8
and r9, 1 ; We only care about carry flag here

stc
rcl dl, 2
lahf
mov r10w, ax
shr r10, 8
and r10, 1 ; We only care about carry flag here

clc
rcl sil, 2
lahf
mov r11w, ax
shr r11, 8
and r11, 1 ; We only care about carry flag here

hlt
