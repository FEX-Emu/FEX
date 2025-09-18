
%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Env": { "FEX_MAXINST" : "41010", "FEX_MULTIBLOCK": "1", "FEX_TSOENABLED": "0" }
}
%endif

; FEX-Emu had a bug where it tried encoding too large of a conditional branch offset.

%macro TooLargeConditionalBranch 0
  ; Stresses ARM64's b.cc/cbz/cbnz branch target of +-1MB.
  jmp %%top
  %%top:

  lea rax, [rel data]
  mov rbx, 1
  %rep 37500
    add qword [rel data], rbx
  %endrep
  lea rax, [%%top]
  jnz %%top
%endmacro

mov rax, 0
cmp rax, 0
mov rcx, 0

jz long_jump

TooLargeConditionalBranch

long_jump:
mov rax, 1
hlt

data:
dq 0, 0, 0, 0
