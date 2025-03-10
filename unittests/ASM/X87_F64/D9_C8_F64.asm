%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x3ff0000000000000",
    "RBX":  "0x4000000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

mov eax, 0x3f800000 ; 1.0
mov [rdx + 8 * 0], eax
mov eax, 0x40000000 ; 2.0
mov [rdx + 8 * 1], eax

fld dword [rdx + 8 * 0]
fld dword [rdx + 8 * 1]

fxch

fstp qword [rdx]
mov rax, [rdx]
fstp qword [rdx]
mov rbx, [rdx]

hlt
