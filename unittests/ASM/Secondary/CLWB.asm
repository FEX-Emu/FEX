%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "HostFeatures": ["CLWB"]
}
%endif

mov rdx, 0xe0000000
; Just ensures the code is executed.
clwb [rdx]

mov rax, 1
hlt
