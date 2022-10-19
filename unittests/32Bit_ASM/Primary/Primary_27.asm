%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x12345637"
  },
  "Mode": "32BIT"
}
%endif

mov eax, 0x1234561f
daa
daa
daa
daa
hlt
