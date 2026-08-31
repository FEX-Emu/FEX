%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x80"
  }
}
%endif

; FEX had a bug in the slow path of x87 stack-optimization pass:
; popping the FPU stack never invalidated ST(0)
;
; Note this test only actually exercises the bug under the jit_1 run
; since otherwise the fast path is taken

fninit

fld1
fld1

fyl2x

fxsave [rel .data]

; FTW
; Bit 6 (popped value) must be 0 now
; Bit 7 (result) must be 1
movzx eax, byte [rel .data + 4]

hlt

align 16
.data:
times 64 dq 0
