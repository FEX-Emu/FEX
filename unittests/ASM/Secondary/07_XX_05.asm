%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "HostFeatures": ["SERIALIZE"]
}
%endif

; Just ensures the instruction executes
mov rax, 1
serialize
hlt
