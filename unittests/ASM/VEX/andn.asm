%ifdef CONFIG
{
  "RegData": {
      "RAX": "0xFFFFFFFFFFFFFFFF",
      "RBX": "0",
      "RCX": "0xFFFFFFFF",
      "RDX": "0"
  },
  "HostFeatures": ["BMI1"]
}
%endif

mov rax, 0
mov rbx, -1
andn rax, rax, rbx
andn rbx, rbx, rax

mov rcx, 0
mov rdx, -1
andn ecx, ecx, edx
andn edx, edx, ecx

hlt
