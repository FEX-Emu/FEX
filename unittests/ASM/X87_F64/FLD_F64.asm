%ifdef CONFIG
{
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" },
  "RegData": {
    "RAX": "0x40490fdb",
    "RBX": "0x4008000000000000"
  }
}
%endif

lea rbp, [rel data]
mov rdx, 0xe0000000

fld dword [rbp]
fst dword [rdx]

xor rax, rax
mov eax, [rdx]

fld qword [rbp + 4]
fst qword [rdx]

mov rbx, [rdx]

hlt

align 8
data:
  dd 0x40490fdb
  dq 0x4008000000000000
