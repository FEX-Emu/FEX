%ifdef CONFIG
{
  "RegData": {
    "R14": "0xffffffffffffffff",
    "R13": "0xffffffffffffff7f",
    "R12": "0xffffffffffffff3f",
    "R11": "0xffffffffffffff1f",
    "R10": "0xffffffffffffff0f",
    "R9": "0xffffffffffffff07",
    "R8": "0xffffffffffffff03",
    "RBP": "0xffffffffffffff01",
    "RSP": "0xffffffffffffff00",
    "RDI": "0xffffffffffffff00",
    "RSI": "0xffffffffffffff00"
  }
}
%endif

mov r14, -1
mov r13, -1
mov r12, -1
mov r11, -1
mov r10, -1
mov r9, -1
mov r8, -1
mov rbp, -1
mov rsp, -1
mov rdi, -1
mov rsi, -1

shr r14b, 0
shr r13b, 1
shr r12b, 2
shr r11b, 3
shr r10b, 4
shr r9b, 5
shr r8b, 6
shr bpl, 7
shr spl, 8
shr dil, 9
shr sil, 10

hlt
