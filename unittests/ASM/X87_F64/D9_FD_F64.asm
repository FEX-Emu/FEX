%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x43000000",
    "RBX":  "0x40b00000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rcx, 0xe0000000

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

fscale

; Store as single precision to get around precision issues

fstp dword [rcx]
mov eax, [rcx]

fst dword [rcx]
mov ebx, [rcx]

hlt

align 8
data:
  dt 4.0
  dq 0

data2:
  dt 5.5
  dq 0
