%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4010000000000000",
    "RBX": "0x4000000000000000",
    "RCX": "0x4014000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif


mov rdx, 0xe0000000

mov rax, 0x3ff0000000000000 ; 1.0
mov [rdx + 8 * 0], rax
mov rax, 0x4000000000000000 ; 2.0
mov [rdx + 8 * 1], rax
mov rax, 0x4010000000000000 ; 4.0
mov [rdx + 8 * 2], rax

fld qword [rdx + 8 * 0]
fld qword [rdx + 8 * 1]
fld qword [rdx + 8 * 2]

; fadd st(i), st(0)
fadd st2, st0

fstp qword [rdx]
mov rax, [rdx]
fstp qword [rdx]
mov rbx, [rdx]
fstp qword [rdx]
mov rcx, [rdx]

hlt
