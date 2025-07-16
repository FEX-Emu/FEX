%ifdef CONFIG
{
  "RegData": {
    "RAX": "0"
  }
}
%endif

; FEX-Emu had a bug where an `int 0x80` instruction that never gets executed would cause the emulator to assert.
; This was due to multiblock's static analysis finding a 32-bit syscall in some code down a branch that would never execute.
; Ensure this doesn't take down the emulator by doing something similar.
mov rax, 0
cmp rax, 0

je .end

mov rax, 1
int 0x80

.end:
hlt
