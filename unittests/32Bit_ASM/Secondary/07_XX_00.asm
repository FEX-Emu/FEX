%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "0x00000000FFFE0000"
  },
  "Mode": "32BIT"
}
%endif

sgdt [rel data]

movzx eax, word [rel data]
mov ebx, dword [rel data + 2]
hlt

align 4096
data:
; Limit
dw 0
; Base
dd 0
