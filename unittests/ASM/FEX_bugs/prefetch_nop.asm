%ifdef CONFIG
{
}
%endif

; FEX-Emu had a bug where the `prefetch NTA/T0/T1/T2` encoding range wasn't handling nops correctly.
; While the first four instructions in the group16 encoding range is considered prefetch,
; If the destination is encoded as a register then it is a nop instead.
; This is to preserve legacy instruction behaviour where this entire group was considered NOP encoding.
; FEX was accidentally declaring these encoded nop instructions to be invalid.

; nop ebx that overlaps `prefetch t0` instruction.
; Just ensure it executes. Seen in `Devil May Cry 4`
db 0x0f, 0x18, 0xcb

hlt
