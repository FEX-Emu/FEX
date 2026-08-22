%ifdef CONFIG
{
  "RegData": {
      "RAX": "0",
      "RCX": "1"
  },
  "HostFeatures": ["BMI1"]
}
%endif

mov rbx, 0xFFFFFFFF00000000
blsr eax, ebx
setc cl
movzx rcx, cl

hlt
