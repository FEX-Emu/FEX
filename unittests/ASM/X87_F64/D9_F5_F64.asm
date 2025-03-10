%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0xbf666666",
    "RBX":  "0x40400000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rcx, 0xe0000000

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

fprem1

; Store as single precision to get around precision issues

fstp dword [rcx]
mov eax, [rcx]

fst dword [rcx]
mov ebx, [rcx]

hlt

align 8
data:
  dt 3.0
  dq 0
data2:
  dt 5.1
  dq 0
