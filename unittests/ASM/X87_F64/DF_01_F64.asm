%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x2",
    "RBX": "0x3ff0000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data3]
fisttp word [rdx + 8 * 0]

mov ax, word [rdx + 8 * 0]

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

fst qword [rdx + 8 * 0]
mov rbx, [rdx + 8 * 0]

hlt

align 4096
data:
  dt 2.0
  dq 0
data2:
  dt 1.0
  dq 0
data3:
  dq -1
  dq -1
