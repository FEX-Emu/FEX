%ifdef CONFIG
{
  "RegData": {
    "R15": "0xFFFFFFFFFFFFFFFF",
    "R14": "0x4141414141410000",
    "R13": "0",
    "R12": "0"
  }
}
%endif

mov cl, 0
mov r15, -1
mov r14, 0x4141414141410000
mov r13, 0
mov r12, 0

shrd r14w, r15w, cl
shrd r13d, r15d, cl
shrd r12, r15, cl

hlt
