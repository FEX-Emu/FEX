%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Env": { "FEX_MAXINST" : "41010", "FEX_MULTIBLOCK": "1", "FEX_TSOENABLED": "0" }
}
%endif

; FEX-Emu had a bug where it tried to encode too large of a tbz/tbnz offset.

%macro TooConditionalBitBranch 0
  ; Stresses ARM64's tbz/tbnz branch target of +-32KB.
  jmp %%top
  %%top:

  lea rax, [rel data]
  mov rbx, 1
  %rep 2000
    add qword [rel data], rbx
  %endrep
  lea rax, [%%top]
  jpe %%top
%endmacro

mov rax, 0
cmp rax, 0
mov rcx, 0

jz long_jump

TooConditionalBitBranch

long_jump:
mov rax, 1
hlt

data:
dq 0, 0, 0, 0
