%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

mov rdx, 0xe0000000
; Just ensures the code is executed.
clwb [rdx]

mov rax, 1
hlt
