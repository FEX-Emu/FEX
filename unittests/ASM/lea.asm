%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "RAX": "0x1BD5B7DDE",
    "RBX": "0x0DEADBF18"
  }
}
%endif

mov r15, 0xDEADBEEF
mov r14, 0x5

lea rax, [r15*2]
lea rbx, [r15+r14*8 + 1]

hlt
