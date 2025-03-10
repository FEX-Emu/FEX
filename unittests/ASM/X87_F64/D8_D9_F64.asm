%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x3ff0000000000000",
    "RBX":  "0x3ff0000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif


mov rdx, 0xe0000000

; Only tests pop behaviour
fld1
fldz
fcomp
fld1

fstp qword [rdx]
mov rax, [rdx]
fstp qword [rdx]
mov rbx, [rdx]

hlt
