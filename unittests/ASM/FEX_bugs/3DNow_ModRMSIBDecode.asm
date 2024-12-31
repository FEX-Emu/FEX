%ifdef CONFIG
{
  "RegData": {
    "MM0":  "0x3f800000bf800000"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

; FEX-Emu had a bug with 3DNow! ModRM decoding when the source was SIB encoded.
; This would result in a crash in the frontend instruction decoding.
; Generate a 3DNow! instruction that uses SIB encoding to ensure this code path is tested.
lea rax, [rel data1]
mov rbx, 0
pi2fw mm0, [rbx * 8 + rax + 0]

hlt

align 8
data1:
dw -1
dw 0xFF
dw 1
dw 0xFF
