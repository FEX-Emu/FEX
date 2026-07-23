%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "RAX": "0x20"
  }
}
%endif

; A decoded block containing an unsupported operation must not poison later,
; independently reachable blocks in the same multiblock unit.
; The conditional branch is always taken, so the invalid block is compiled
; but never executed.
xor eax, eax
test eax, eax
jz valid_block

invalid_block:
mov fs, ax

valid_block:
jmp success

wrong_fallthrough:
mov rax, 0x10
hlt

success:
mov rax, 0x20
hlt
