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
mov r15, 0
mov r14, 0x4141414141410002
mov r13, 0xFFFFFFFF00000002
mov r12, 0x0000000000000002

mov rax, 0
mov rbx, 0
mov rdx, 0
mov rsi, 1

shrd r14w, r15w, cl
cmovz rax, rsi
shrd r13d, r15d, cl
cmovz rbx, rsi
shrd r12, r15, cl
cmovz rdx, rsi

hlt
