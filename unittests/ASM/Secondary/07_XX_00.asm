%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "0xFFFFFFFFFFFE0000"
  }
}
%endif

sgdt [rel data]

movzx rax, word [rel data]
mov rbx, qword [rel data + 2]
hlt

align 4096
data:
; Limit
dw 0
; Base
dq 0
