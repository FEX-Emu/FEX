%ifdef CONFIG
{
  "MemoryRegions": {
    "0xf0000000": "4096",
    "0xf1000000": "4096"
  }
}
%endif

; FEX-Emu had a bug where a backwards repeating string operation would read past the end of a mapped page.
; This was encountered in https://github.com/FEX-Emu/FEX/pull/3478.
; To ensure we don't read past a page with `rep stos` and `rep movs`, map two disparate pages and copy the entire page.
; If FEX tries reading past the ends of either then it will fault.
%macro do_rep_op 2
  jmp %%1
  %%1:

  cld
  mov rax, r13
  mov rdi, r14
  mov rsi, r15
  mov rcx, (4096 / %2)
  rep %1
%endmacro

%macro do_backward_rep_op 2
  jmp %%1
  %%1:

  std
  mov rax, r13
  mov rdi, r14
  mov rsi, r15
  add rdi, (4096 - %2)
  add rsi, (4096 - %2)
  mov rcx, (4096 / %2)
  rep %1
%endmacro

mov r15, 0xf000_0000
mov r14, 0xf100_0000
mov r13, 0x41424344454647

do_rep_op stosb, 1
do_rep_op stosw, 2
do_rep_op stosd, 4
do_rep_op stosq, 8

do_backward_rep_op stosb, 1
do_backward_rep_op stosw, 2
do_backward_rep_op stosd, 4
do_backward_rep_op stosq, 8

do_rep_op movsb, 1
do_rep_op movsw, 2
do_rep_op movsd, 4
do_rep_op movsq, 8

do_backward_rep_op movsb, 1
do_backward_rep_op movsw, 2
do_backward_rep_op movsd, 4
do_backward_rep_op movsq, 8

hlt

