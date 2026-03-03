%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x3ff0000000000000",
    "RBX":  "0x8000000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rcx, 0xe0000000

lea rdx, [rel data]
fld qword [rdx]

fptan

; ST(0) = 1.0, ST(1) = tan(-0.0) = -0.0
fstp qword [rcx]
mov rax, [rcx]

fstp qword [rcx]
mov rbx, [rcx]

hlt

align 8
data:
  dq 0x8000000000000000 ; -0.0
