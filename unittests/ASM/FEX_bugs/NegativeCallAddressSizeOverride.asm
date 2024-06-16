%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; FEX had a bug with relative call instructions.
; It was incorrectly truncating the immediate displacement based on address size override AND operand size override.
; Address size override doesn't actually change immediate representation on the call instruction.

mov rsp, 0xe000_1000
mov rax, 0

jmp .after
.test:
mov rax, 1
hlt

.after:
a32 call .test

hlt
