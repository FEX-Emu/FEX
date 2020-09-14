%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x00000003",
    "RCX": "0x00000002",
    "RDX": "0x00000001",
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
mov rdx, 0x80000000
mov rsi, 0x80000000
mov r15, 1

stc
rcl ebx, 1
mov r8, 0
cmovo r8, r15 ; We only care about OF here

clc
rcl ecx, 1
mov r9, 0
cmovo r9, r15 ; We only care about OF here

stc
rcl edx, 1
mov r10, 0
cmovo r10, r15 ; We only care about OF here

clc
rcl esi, 1
mov r11, 0
cmovo r11, r15 ; We only care about OF here

hlt
