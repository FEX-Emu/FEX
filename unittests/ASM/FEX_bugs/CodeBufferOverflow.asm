%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Env": { "FEX_MAXINST" : "41010", "FEX_MULTIBLOCK": "1", "FEX_TSOENABLED": "0" }
}
%endif

; FEX had a bug where its JIT heuristic wouldn't catch all cases, and allocations would overflow and crash.

%macro OverflowBuffer 0
  %rep 256
  ; This instruction is absolutely abysmal under FEX.
  ; Easily stresses our heuristic for block sizes
  rep movsq
  %endrep
%endmacro

mov rax, 0
cmp rax, 0
mov rcx, 0
mov rdi, 1
mov rsi, 2

jz long_jump

OverflowBuffer

long_jump:
mov rax, 1
hlt

data:
dq 0, 0, 0, 0
