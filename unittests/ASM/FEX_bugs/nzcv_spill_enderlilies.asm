%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "XMM0": ["0", "0"]
  },
  "HostFeatures": ["SHA"]
}
%endif

; FEX-Emu has a bug around NZCV flags getting spilled and filled.
; The bug comes down to NZCV actually being 32-bit but our IR incorrectly assumed that all flags were 8-bit.
; Once a spill situation happened, it would only store and reload the lower 8-bits of the NZCV flag which wasn't correct.
; This caused this code to infinite loop and read past memory and crash.

; Code found from Ender Lilies in their `sha1_block_data_order` function which is significantly longer than this snippit.
lea rsi, [rel .data_vecs]
mov rax, 1

; Break visibility
jmp loop_top
loop_top:

; Decrement counter.
dec     rax

; Load rsi + 0x40 in to rbx
lea     rbx, [rsi+0x40]

; Move rbx in to rsi, incrementing the pointer by 64-bytes if rax isn't zero.
cmovne  rsi, rbx

; Do a sha1rnds4, which uses enough temporaries to spill NZCV which picks up a crash.
sha1rnds4 xmm0, xmm0, 0x0

; This memory access will crash once we loop too many times.
movdqu  xmm0, [rsi]

; Jump back to the top
jne     loop_top

hlt

.data_vecs:
dq 0, 0, 0, 0
dq 0, 0, 0, 0
dq 0, 0, 0, 0
dq 0, 0, 0, 0
dq 0, 0, 0, 0
dq 0, 0, 0, 0
