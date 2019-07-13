%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "RAX": "0xFFFFFFFFFFFFFFFE",
    "RDX": "1",
    "RBX": "1"
  }
}
%endif
mov rax, -1
mov rdx, 0
mov rbx, 1

mul rbx

hlt
