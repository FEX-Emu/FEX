%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x8000000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rcx, 0xe0000000

lea rdx, [rel data]
fld qword [rdx]

fsin

fstp qword [rcx]
mov rax, [rcx]

hlt

align 8
data:
  dq 0x8000000000000000 ; -0.0
