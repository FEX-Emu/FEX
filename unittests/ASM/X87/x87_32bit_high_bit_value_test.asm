%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x7F7FFFFF", "0", "0", "0"]
  }
}
%endif

mov rsp, 0xe0000040

; Test a 32-bit value with many high bits set (max normal float)
; This could be misinterpreted as having NaN-like properties
; when treated as 64-bit, potentially triggering incorrect ORing.

finit
lea rdx, [rel data]
fld dword [rdx]           ; Load max normal 32-bit float
fstp dword [rdx + 16]     ; Store back as 32-bit

; Verify the value is unchanged
movss xmm0, [rdx + 16]

hlt

align 4096
data:
  dd 0x7F7FFFFF             ; Max normal 32-bit float
  dd 0
  dd 0
  dd 0                      ; space for result
