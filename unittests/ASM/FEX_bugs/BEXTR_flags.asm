%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RCX": "0x5a"
  },
  "HostFeatures": ["BMI1"]
}
%endif

mov rcx, 0x8f635a775ad3b9b4
mov esi, 0x3018
bextr ecx, ecx, esi
cmp rcx, 0x5a
jne .bad

.good:
mov rax, 0
hlt

.bad:
mov rax, 1
hlt
