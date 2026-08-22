%ifdef CONFIG
{
  "RegData": {
      "RAX": "0xFFFFFFFF",
      "RCX": "1"
  },
  "HostFeatures": ["BMI1"]
}
%endif

mov rbx, 0xFFFFFFFF00000000
blsmsk eax, ebx
setc cl
movzx rcx, cl

hlt
