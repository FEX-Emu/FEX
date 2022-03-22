%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x4010000000000000",
    "RBX":  "0x0",
    "RCX":  "0x4000000000000000",
    "RDX": "0x3ff0000000000000"
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

; Do Some stack shenanigans
fldz
fincstp
fdecstp

fld qword [rdx + 8 * 2]

; dump stack to registers
mov r9, 0xe0000000
fstp qword [r9]
mov rax, [r9]
fstp qword [r9]
mov rbx, [r9]
fstp qword [r9]
mov rcx, [r9]
fstp qword [r9]
mov rdx, [r9]


hlt
