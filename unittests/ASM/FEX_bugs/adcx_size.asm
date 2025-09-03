%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x000000007ee544ac",
    "RBX": "0x0fb22768a2cf00bb",
    "RCX": "0x00000000e19be77f",
    "RDX": "0x06726399b9f09d2f",
    "RSI": "0xe544b42838dd404d",
    "RDI": "0x6d78590ca1418bd1",
    "RSP": "0x20bfe50ddcfce881",
    "RBP": "0x56c870e2dcbf6522"
  },
  "HostFeatures": ["ADX"]
}
%endif

mov rax, 0x6B11A609DC1643F1
mov rbx, 0x0FB22768A2CF00BB
mov rcx, 0x48E1BB8327AB4A4F
mov rdx, 0x06726399B9F09D2F
mov rsi, 0x77CC5B1B979BB47C
mov rdi, 0x6D78590CA1418BD1
mov rsp, 0xC9F7742B003D835E
mov rbp, 0x56C870E2DCBF6522

; 32-bit clc
clc
adcx eax, ebx

; 32-bit stc
stc
adcx ecx, edx

; 64-bit clc
clc
adcx rsi, rdi

; 64-bit stc
stc
adcx rsp, rbp

hlt
