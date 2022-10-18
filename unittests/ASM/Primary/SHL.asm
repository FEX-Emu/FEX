%ifdef CONFIG
{
  "RegData": {
    "R14": "0xffffffffffffffff",
    "R13": "0xfffffffffffffffe",
    "R12": "0xfffffffffffffffc",
    "R11": "0xfffffffffffffff8",
    "R10": "0xfffffffffffffff0",
    "R9": "0xffffffffffffffe0",
    "R8": "0xffffffffffffffc0",
    "RBP": "0xffffffffffffff80",
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

shl r14b, 0
shl r13b, 1
shl r12b, 2
shl r11b, 3
shl r10b, 4
shl r9b, 5
shl r8b, 6
shl bpl, 7
shl spl, 8
shl dil, 9
shl sil, 10

hlt
