%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; FEX-Emu had a bug where segment registers prefixed with `o16` were still pushing and popping a full 64-bits to the stack.
; With o16 it is supposed to only operate on 16-bits. Run through the instructions and just ensure it is pushing/popping the correct amount.

; Need to save and restore fs/gs base on this test.
rdfsbase rax
mov [rel .data_segment], rax

rdgsbase rax
mov [rel .data_segment + 8], rax

mov rax, 0
mov rbx, 0xe000_0ffe
mov rcx, 0xe000_1000
mov rsp, 0xe000_1000

; Ensure 16-bit FS/GS push/pops are the correct size.
o16 push fs
cmp rsp, rbx
jne .bad

o16 pop fs
cmp rsp, rcx
jne .bad

o16 push gs
cmp rsp, rbx
jne .bad

o16 pop gs
cmp rsp, rcx
jne .bad

; Check 64-bit as well.
mov rbx, 0xe000_0ff8

push fs
cmp rsp, rbx
jne .bad

pop fs
cmp rsp, rcx
jne .bad

push gs
cmp rsp, rbx
jne .bad

pop gs
cmp rsp, rcx
jne .bad

mov rax, 1
jmp .end

.bad:
mov rax, 0

.end:
mov rbx, [rel .data_segment]
wrfsbase rbx

mov rbx, [rel .data_segment + 8]
wrgsbase rbx

hlt

align 4096
.data_segment:
dq 0
dq 0
