%ifdef CONFIG
{
  "RegData": {
      "RAX": "0xbeef",
      "RCX": "1"
  }
}
%endif

mov eax, 1
mov edx, 0xbeef
shld ax, dx, 16
setc cl
movzx rcx, cl

hlt
