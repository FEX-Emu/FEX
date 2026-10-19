%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Just ensures the instruction executes
mov rax, 1
serialize
hlt
