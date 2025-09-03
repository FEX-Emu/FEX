%ifdef CONFIG
{
  "RegData": {
    "RSP": "0x48",
    "RBP": "0x47",
    "RSI": "0x46",
    "RDI": "0x45"
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

lea rdx, [rel data]

movaps xmm0, [rel data]

; Special testing for storing in to registers rsp, rbp, rsi, rdi
; These registers are in the 'high' modrm.reg encoding which can
; mean ah/ch/dh/bh or rsp/rbp/rsi/rdi depending on instruction

mov rsp, -1
mov rbp, -1
mov rsi, -1
mov rdi, -1

pextrb rsp, xmm0, 0
pextrb rbp, xmm0, 1
pextrb rsi, xmm0, 2
pextrb rdi, xmm0, 3

hlt

align 16
data:
dq 0x4142434445464748
