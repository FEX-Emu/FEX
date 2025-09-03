%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "HostFeatures": ["RDPID"]
}
%endif

mov rax, 0
mov rbx, 0x4142434445464748
mov rcx, 0x4142434445464748
rdpid ebx

cmp rbx, rcx
setne al

hlt
