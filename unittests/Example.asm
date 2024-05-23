%ifdef CONFIG
{
  "Match": "All"
}
%endif

loop:
    lea    r9,[r11+r11*1]
    mov    r10,r11
    cmp    r9,rdi
    ja     exit
    cmp    r9,rdi
    jae    b8
    mov    r13,qword [rsi+r9*8]
    mov    r11,r9
    or     r11,0x1
    cmp    r13,qword [rsi+r11*8]
    jl     b5
    mov    r11,r9
b5:
    mov    r9,r11
b8:
    mov    r13,qword [rsi+r10*8]
    mov    rbp,qword [rsi+r9*8]
    mov    r11,r8
    cmp    r13,rbp
    jge    loop
    mov    qword [rsi+r9*8],r13
    mov    qword [rsi+r10*8],rbp
    mov    r11,r9
    jmp    loop

exit:

;mov     rdx, rax
;mov     rcx, rax
;mov     r14, rsi
;add     rax, 0x1
;shr     rdx, 0x6
;and     ecx, 0x3f
;shl     r14, cl
;xor     qword [rbx+rdx*8], r14
;cmp     rdi, rax

ret
