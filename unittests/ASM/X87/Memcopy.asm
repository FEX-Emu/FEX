%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3ff8000000000000",
    "RBX": "0x3ff8000000000000"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0x100000000
mov rax, 0x3ff8000000000000 ; 1.5
mov [rdx], rax

fld qword [rdx]
fstp qword [rdx + 8]

mov rbx, [rdx + 8]
hlt
