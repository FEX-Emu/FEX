%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x4000",
    "RDI": "0x0000",
    "RDX": "0x6000",
    "RSI": "0x2000",
    "R8":  "0x1",
    "R9":  "0x1",
    "R10": "0x0",
    "R11": "0x0"
  }
}
%endif

mov rbx, 0x0002
mov rdi, 0x0002
mov rdx, 0x8000
mov rsi, 0x8000
mov rcx, 2

stc
rcr bx, cl
lahf
mov r8w, ax
shr r8, 8
and r8, 1 ; We only care about carry flag here

clc
rcr di, cl
lahf
mov r9w, ax
shr r9, 8
and r9, 1 ; We only care about carry flag here

stc
rcr dx, cl
lahf
mov r10w, ax
shr r10, 8
and r10, 1 ; We only care about carry flag here

clc
rcr si, cl
lahf
mov r11w, ax
shr r11, 8
and r11, 1 ; We only care about carry flag here

hlt
