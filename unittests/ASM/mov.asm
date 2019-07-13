%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "RAX": "0xFFFFFFFFFFFFFFD1",
    "RBX": "0xFFFFFFFFFFFFDAD1",
    "RCX": "0xBAD0DAD1",
    "RDX": "0xDEADBEEFBAD0DAD1",
    "R15": "0xDEADBEEFBAD0DAD1"
  }
}
%endif

mov rax, -1
mov rbx, -1
mov rcx, -1

mov r15, qword 0xDEADBEEFBAD0DAD1
mov rdx, qword 0xDEADBEEFBAD0DAD1

;mov al, dl
;mov bx, dx
;mov ecx, edx
;mov al, -1
;mov ax, -1
;mov eax, -1
;mov rax, qword -1
;mov rax, 0
;mov al, al
;mov rbx, -1
;mov bx, ax
;mov ax, ax
;mov ax, ax
;mov eax, eax
;mov rax, rax
hlt
