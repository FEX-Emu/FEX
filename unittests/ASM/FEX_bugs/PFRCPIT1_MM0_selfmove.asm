%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xff",
    "RBX": "0x1122334455667788",
    "RCX": "0x000000000000ffff"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

; PFRCPIT1 mm0, mm0 (along with PFRSQIT1/PFRCPIT2) was treated as a Nop
; skipping the MMX state transition Real MMX writes always mark the
; destination valid and force its aliased x87 sign+exponent to 0xffff
fxrstor [rel .fxsave_data]
pfrcpit1 mm0, mm0

fxsave [rel .save]
movzx eax, byte [rel .save + 4]
mov rbx, qword [rel .save + 32]
movzx ecx, word [rel .save + 40]

hlt

align 16
.fxsave_data:
dw 0x037f       ; FCW
dw 0x0000       ; FSW
dw 0x0001       ; FTW
times 26 db 0
dq 0x1122334455667788   ; MM0 significand
dw 0x0000               ; MM0 sign+exponent
times 470 db 0

align 16
.save:
times 64 dq 0
