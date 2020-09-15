%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x8000",
    "RDI": "0x0000",
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
mov rdi, 0x0001
mov rdx, 0x8000
mov rsi, 0x8000
mov r15, 1
mov rcx, 1

stc
rcr bx, cl
mov r8, 0
cmovo r8, r15 ; We only care about OF here

clc
rcr di, cl
mov r9, 0
cmovo r9, r15 ; We only care about OF here

stc
rcr dx, cl
mov r10, 0
cmovo r10, r15 ; We only care about OF here

clc
rcr si, cl
mov r11, 0
cmovo r11, r15 ; We only care about OF here

hlt
