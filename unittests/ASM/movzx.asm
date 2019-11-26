%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "RBX": "0xFFFFFFFFFFFF00D1",
    "RCX": "0x00000000000000D1",
    "RDX": "0xDAD1",
    "RDI": "0xDAD1"
  }
}
%endif

mov rax, qword 0xDEADBEEFBAD0DAD1

mov rbx, -1
mov rcx, -1
mov rdx, -1
mov rdi, -1

movzx bx,  al ; 8bit-> 16bit
movzx ecx, al ; 8bit-> 32bit
movzx edx, ax ; 16bit-> 32bit
movzx rdi, ax ; 16bit -> 64bit

hlt