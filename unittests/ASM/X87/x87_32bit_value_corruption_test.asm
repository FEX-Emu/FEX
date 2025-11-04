%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x3F800000", "0", "0", "0"]
  }
}
%endif

mov rsp, 0xe0000040

; Test that a simple 32-bit value (1.0) doesn't get corrupted
; when stored through the x87 stack in full precision mode.
; The buggy SilenceNaN extracts 64 bits and ORs with bit 51,
; which could corrupt the value.

finit
lea rdx, [rel data]
fld dword [rdx]           ; Load 1.0 as 32-bit
fstp dword [rdx + 16]     ; Store back as 32-bit

; Load result and verify it's still 1.0
movss xmm0, [rdx + 16]

hlt

align 4096
data:
  dd 0x3F800000             ; 1.0 in 32-bit float
  dd 0
  dd 0
  dd 0                      ; space for result
