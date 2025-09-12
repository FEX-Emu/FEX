%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "MemoryRegions": {
    "0x100000000": "4096",
    "0x100002000": "4096"
  }
}
%endif

mov r15, 0x100001000
mov r14, 0x100002000
mov rax, 0

%include "modrm_oob_macros.mac"

; Secondary Opsize
rw3 movupd, 16, xmm0
rw3 movlpd, 8, xmm0
r3 unpcklpd, 16, xmm0
r3 unpckhpd, 16, xmm0
rw3 movhpd, 8, xmm0
rw3 movapd, 16, xmm0
r3 cvtpi2pd, 8, xmm0
w3 movntpd, 16, xmm0
r3 cvttpd2pi, 16, mm0
r3 cvtpd2pi, 16, mm0
r3 ucomisd, 8, xmm0
r3 comisd, 8, xmm0

r3 sqrtpd, 16, xmm0
r3 andpd, 16, xmm0
r3 andnpd, 16, xmm0
r3 orpd, 16, xmm0
r3 xorpd, 16, xmm0
r3 addpd, 16, xmm0
r3 mulpd, 16, xmm0
r3 cvtpd2ps, 16, xmm0
r3 cvtps2dq, 16, xmm0
r3 subpd, 16, xmm0
r3 minpd, 16, xmm0
r3 divpd, 16, xmm0
r3 maxpd, 16, xmm0

r3 punpcklbw, 16, xmm0
r3 punpcklwd, 16, xmm0
r3 punpckldq, 16, xmm0
r3 packsswb, 16, xmm0
r3 pcmpgtb, 16, xmm0
r3 pcmpgtw, 16, xmm0
r3 pcmpgtd, 16, xmm0
r3 packuswb, 16, xmm0
r3 punpckhbw, 16, xmm0
r3 punpckhwd, 16, xmm0
r3 punpckhdq, 16, xmm0
r3 packssdw, 16, xmm0
r3 punpcklqdq, 16, xmm0
r3 punpckhqdq, 16, xmm0

rw3 movdqa, 16, xmm0

r4 pshufd, 16, xmm0, 1

r3 pcmpeqb, 16, xmm0
r3 pcmpeqw, 16, xmm0
r3 pcmpeqd, 16, xmm0
r3 haddpd, 16, xmm0
r3 hsubpd, 16, xmm0

r4 cmppd, 16, xmm0, 0
r4 cmppd, 16, xmm0, 1
r4 cmppd, 16, xmm0, 2
r4 cmppd, 16, xmm0, 3
r4 cmppd, 16, xmm0, 4
r4 cmppd, 16, xmm0, 5
r4 cmppd, 16, xmm0, 6
r4 cmppd, 16, xmm0, 7

r4 pinsrw, 2, xmm0, 0
w4 pextrw, 2, xmm0, 0

r4 shufpd, 16, xmm0, 0
r3 addsubpd, 16, xmm0
r3 psrlw, 16, xmm0
r3 psrld, 16, xmm0
r3 psrlq, 16, xmm0
r3 paddq, 16, xmm0
r3 psubusb, 16, xmm0
r3 psubusw, 16, xmm0
r3 pminub, 16, xmm0
r3 pand, 16, xmm0
r3 paddusb, 16, xmm0
r3 paddusw, 16, xmm0
r3 pmaxub, 16, xmm0
r3 pandn, 16, xmm0

r3 pavgb, 16, xmm0
r3 psraw, 16, xmm0
r3 psrad, 16, xmm0
r3 pavgw, 16, xmm0
r3 pmulhuw, 16, xmm0
r3 pmulhw, 16, xmm0
r3 cvttpd2dq, 16, xmm0
w3 movntdq, 16, xmm0
r3 psubsb, 16, xmm0
r3 pminsw, 16, xmm0
r3 por, 16, xmm0
r3 paddsb, 16, xmm0
r3 paddsw, 16, xmm0
r3 pmaxsw, 16, xmm0
r3 pxor, 16, xmm0
r3 psllw, 16, xmm0
r3 pslld, 16, xmm0
r3 psllq, 16, xmm0
r3 pmuludq, 16, xmm0
r3 pmaddwd, 16, xmm0
r3 psadbw, 16, xmm0
r3 psubb, 16, xmm0
r3 psubw, 16, xmm0
r3 psubd, 16, xmm0
r3 psubq, 16, xmm0
r3 paddb, 16, xmm0
r3 paddw, 16, xmm0
r3 paddd, 16, xmm0

; Done
mov rax, 1
hlt
