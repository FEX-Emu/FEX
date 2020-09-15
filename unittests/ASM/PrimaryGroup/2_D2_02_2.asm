%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x03",
    "RDI": "0x02",
    "RDX": "0x01",
    "RSI": "0x00",
    "R8":  "0x0",
    "R9":  "0x0",
    "R10": "0x1",
    "R11": "0x1"
  }
}
%endif

mov rbx, 0x01
mov rdi, 0x01
mov rdx, 0x80
mov rsi, 0x80
mov rcx, 1
mov r15, 1

stc
rcl bl, cl
mov r8, 0
cmovo r8, r15 ; We only care about OF here

clc
rcl dil, cl
mov r9, 0
cmovo r9, r15 ; We only care about OF here

stc
rcl dl, cl
mov r10, 0
cmovo r10, r15 ; We only care about OF here

clc
rcl sil, cl
mov r11, 0
cmovo r11, r15 ; We only care about OF here

hlt
