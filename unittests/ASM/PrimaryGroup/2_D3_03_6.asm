%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x8000000000000000",
    "RDI": "0x0000000000000000",
    "RDX": "0xC000000000000000",
    "RSI": "0x4000000000000000",
    "R8":  "0x1",
    "R9":  "0x0",
    "R10": "0x0",
    "R11": "0x1"
  }
}
%endif

mov rbx, 0x0000000000000001
mov rdi, 0x0000000000000001
mov rdx, 0x8000000000000000
mov rsi, 0x8000000000000000
mov r15, 1
mov rcx, 1

stc
rcr rbx, cl
mov r8, 0
cmovo r8, r15 ; We only care about OF here

clc
rcr rdi, cl
mov r9, 0
cmovo r9, r15 ; We only care about OF here

stc
rcr rdx, cl
mov r10, 0
cmovo r10, r15 ; We only care about OF here

clc
rcr rsi, cl
mov r11, 0
cmovo r11, r15 ; We only care about OF here

hlt
