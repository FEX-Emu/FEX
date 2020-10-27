%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x37F"
  },
  "Mode": "32BIT"
}
%endif

mov edx, 0xe0000000
fnstcw [edx]
mov eax, 0
mov ax, [edx]

hlt
