%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x02",
    "RBX": "0x02",
    "RCX": "0x02",
    "RDX": "0x02"
  }
}
%endif

mov rax, 0x01
mov rbx, 0x01
mov rcx, 0x01
mov rdx, 0x01

xadd al, al
xadd bx, bx
xadd ecx, ecx
xadd rdx, rdx

hlt
