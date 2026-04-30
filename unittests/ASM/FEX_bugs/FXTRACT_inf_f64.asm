%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x7ff0000000000000",
    "RBX":  "0x7ff0000000000000",
    "RCX":  "0x7ff0000000000000",
    "RDX":  "0xfff0000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

; FXTRACT(+/-Inf) under reduced-precision F64 must produce exp=+Inf with the
; significand preserving the input sign.

finit

fld qword [rel pos_inf]
fxtract
fstp qword [rel sig_pos]
fstp qword [rel exp_pos]

fld qword [rel neg_inf]
fxtract
fstp qword [rel sig_neg]
fstp qword [rel exp_neg]

mov rax, [rel exp_pos]
mov rbx, [rel sig_pos]
mov rcx, [rel exp_neg]
mov rdx, [rel sig_neg]

hlt

align 4096
pos_inf: dq 0x7ff0000000000000
neg_inf: dq 0xfff0000000000000
exp_pos: dq 0
sig_pos: dq 0
exp_neg: dq 0
sig_neg: dq 0
