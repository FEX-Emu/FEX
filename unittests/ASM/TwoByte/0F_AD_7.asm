
%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "RBX": "1",
    "RDX": "1"
  }
}
%endif

mov cl, 2
mov r15, 0xFFFFFFFFFFFFFFF2
mov r14, 0x4141414141410000
mov r13, 0xFFFFFFFF00000000
mov r12, 0x0000000000000000

mov rax, 0
mov rbx, 0
mov rdx, 0
mov rsi, 1

shrd r14w, r15w, cl
cmovs rax, rsi
shrd r13d, r15d, cl
cmovs rbx, rsi
shrd r12, r15, cl
cmovs rdx, rsi

hlt
