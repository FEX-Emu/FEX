%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RBX": "0"
  }
}
%endif

lea rax, [rel .data]
lea rbx, [rel .data_mov]

fld qword [rax]
fstp qword [rbx]

mov rax, [rbx]
mov rbx, [rbx + 8]
hlt

.data:
dq 0x4142434445464748
dq 0x5152535455565758

.data_mov:
dq 0
dq 0
