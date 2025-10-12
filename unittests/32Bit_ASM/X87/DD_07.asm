%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFF3800",
    "RBX": "0xFFFF0000"
  },
  "Mode": "32BIT"
}
%endif

lea edx, [rel .data]

mov eax, -1
mov ebx, -1
fnstsw [edx + 8 * 1]

fld dword [edx + 8 * 0]
fnstsw [edx + 8 * 2]
mov ax, word [edx + 8 * 2]
mov bx, word [edx + 8 * 1]

hlt

align 4096
.data:
dq 0x3f800000
dq 0
dq 0
