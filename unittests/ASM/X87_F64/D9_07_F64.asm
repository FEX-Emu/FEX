%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x37F"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000
fnstcw [rdx]
mov eax, 0
mov ax, [rdx]

hlt
