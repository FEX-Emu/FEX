%ifdef CONFIG
{
  "RegData": {
    "R15": "0x00000000000000cc"
  }
}
%endif

%macro clearof 0
mov r12, 0
ror r12, 1
%endmacro

%macro ofmerge 0
mov r14, 0
mov r13, 1
cmovo r14, r13

or r15, r14
shl r15, 1
%endmacro

mov r15, 0
mov r14, 1

; 1 bit rotate
; ror OF = XOR or two most significant bits of result
clearof
mov rax, 0
mov rcx, 1
ror rax, cl
ofmerge

clearof
mov rax, 1
mov rcx, 1
ror rax, cl
ofmerge

clearof
mov rax, 0x8000000000000000
mov rcx, 1
ror rax, cl
ofmerge

clearof
mov rax, 0x8000000000000001
mov rcx, 1
ror rax, cl
ofmerge

clearof
mov rax, 0
ror rax, 1
ofmerge

clearof
mov rax, 1
ror rax, 1
ofmerge

clearof
mov rax, 0x8000000000000000
ror rax, 1
ofmerge

clearof
mov rax, 0x8000000000000001
ror rax, 1
ofmerge

hlt
