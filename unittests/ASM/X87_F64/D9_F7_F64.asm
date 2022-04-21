%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x4060000000000000",
    "RBX":  "0x4050000000000000",
    "RCX":  "0x4040000000000000",
    "RDX": "0x4030000000000000",
    "R8": "0x4020000000000000",
    "R9": "0x4000000000000000",
    "R10": "0x3ff0000000000000",
    "R11": "0x4070000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3ff0000000000000 ; 1.0
mov [rdx + 8 * 0], rax
mov rax, 0x4000000000000000 ; 2.0
mov [rdx + 8 * 1], rax
mov rax, 0x4020000000000000 ; 4.0
mov [rdx + 8 * 2], rax
mov rax, 0x4030000000000000
mov [rdx + 8 * 3], rax
mov rax, 0x4040000000000000
mov [rdx + 8 * 4], rax
mov rax, 0x4050000000000000
mov [rdx + 8 * 5], rax
mov rax, 0x4060000000000000
mov [rdx + 8 * 6], rax
mov rax, 0x4070000000000000
mov [rdx + 8 * 7], rax

fld qword [rdx + 8 * 0]
fld qword [rdx + 8 * 1]
fld qword [rdx + 8 * 2]
fld qword [rdx + 8 * 3]
fld qword [rdx + 8 * 4]
fld qword [rdx + 8 * 5]
fld qword [rdx + 8 * 6]
fld qword [rdx + 8 * 7]
fincstp

; dump stack to registers
mov r12, 0xe0000000
fstp qword [r12]
mov rax, [r12]
fstp qword [r12]
mov rbx, [r12]
fstp qword [r12]
mov rcx, [r12]
fstp qword [r12]
mov rdx, [r12]
fstp qword [r12]
mov r8, [r12]
fstp qword [r12]
mov r9, [r12]
fstp qword [r12]
mov r10, [r12]
fstp qword [r12]
mov r11, [r12]

hlt
