%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFF3800",
    "RBX": "0xFFFF0000"
  },
  "Mode": "32BIT"
}
%endif

lea edx, [.data]

mov eax, -1
mov ebx, -1
fnstsw ax
mov bx, ax

fld dword [edx + 8 * 0]
fnstsw ax

hlt

.data:
dq 0x3f800000 ; 1.0

