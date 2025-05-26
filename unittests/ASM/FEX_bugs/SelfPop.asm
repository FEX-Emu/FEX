%ifdef CONFIG
{
  "RegData": {
    "RAX": "123",
    "RBX": "456",
    "RCX": "123"
  }
}
%endif

; FEX had a bug merging pops to the same register

; Push some stuff
mov rsp, 0xe0000010
mov rax, 123
mov rbx, 456
push rax
push rbx

; Pop into the same register
pop rcx
pop rcx

; rcx now equals rax
hlt
