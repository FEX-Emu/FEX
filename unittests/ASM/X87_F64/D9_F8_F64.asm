%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x40066666",
    "RBX": "0x40400000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

fprem

fstp dword [rdx + 8]
mov eax, [rdx + 8]
fst dword [rdx + 8]
mov ebx, [rdx + 8]

hlt

align 4096
data:
  dt 3.0
  dq 0
data2:
  dt 5.1
  dq 0
