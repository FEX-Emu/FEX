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

; Secondary table
rw3 movups, 16, xmm0
rw3 movlps, 8, xmm0
r3 unpcklps, 16, xmm0
r3 unpckhps, 16, xmm0
rw3 movhps, 8, xmm0
rw3 movaps, 16, xmm0
r3 cvtpi2ps, 8, xmm0
w3 movntps, 16, xmm0
r3 cvttps2pi, 8, mm0
r3 cvtps2pi, 8, mm0
r3 ucomiss, 4, xmm0
r3 comiss, 4, xmm0

r3 cmovo, 8, rax
r3 cmovo, 4, eax
r3 cmovo, 2, ax

r3 cmovno, 8, rax
r3 cmovno, 4, eax
r3 cmovno, 2, ax

r3 cmovb, 8, rax
r3 cmovb, 4, eax
r3 cmovb, 2, ax

r3 cmovnb, 8, rax
r3 cmovnb, 4, eax
r3 cmovnb, 2, ax

r3 cmovz, 8, rax
r3 cmovz, 4, eax
r3 cmovz, 2, ax

r3 cmovnz, 8, rax
r3 cmovnz, 4, eax
r3 cmovnz, 2, ax

r3 cmovbe, 8, rax
r3 cmovbe, 4, eax
r3 cmovbe, 2, ax

r3 cmovnbe, 8, rax
r3 cmovnbe, 4, eax
r3 cmovnbe, 2, ax

r3 cmovs, 8, rax
r3 cmovs, 4, eax
r3 cmovs, 2, ax

r3 cmovns, 8, rax
r3 cmovns, 4, eax
r3 cmovns, 2, ax

r3 cmovp, 8, rax
r3 cmovp, 4, eax
r3 cmovp, 2, ax

r3 cmovnp, 8, rax
r3 cmovnp, 4, eax
r3 cmovnp, 2, ax

r3 cmovl, 8, rax
r3 cmovl, 4, eax
r3 cmovl, 2, ax

r3 cmovnl, 8, rax
r3 cmovnl, 4, eax
r3 cmovnl, 2, ax

r3 cmovle, 8, rax
r3 cmovle, 4, eax
r3 cmovle, 2, ax

r3 cmovnle, 8, rax
r3 cmovnle, 4, eax
r3 cmovnle, 2, ax

r3 sqrtps, 16, xmm0
r3 rsqrtps, 16, xmm0
r3 rcpps, 16, xmm0
r3 andps, 16, xmm0
r3 andnps, 16, xmm0
r3 orps, 16, xmm0
r3 xorps, 16, xmm0
r3 addps, 16, xmm0
r3 mulps, 16, xmm0
r3 cvtps2pd, 8, xmm0
r3 cvtdq2ps, 16, xmm0
r3 subps, 16, xmm0
r3 minps, 16, xmm0
r3 divps, 16, xmm0
r3 maxps, 16, xmm0
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

rw3 movd, 4, mm0
rw3 movq, 8, mm0
r4 pshufw, 8, mm0, 0
r3 pcmpeqb, 8, mm0
r3 pcmpeqw, 8, mm0
r3 pcmpeqd, 8, mm0

rw3 movd, 4, xmm0
rw3 movq, 8, xmm0

w2 seto, 1
w2 setno, 1
w2 setb, 1
w2 setnb, 1
w2 setz, 1
w2 setnz, 1
w2 setbe, 1
w2 setnbe, 1
w2 sets, 1
w2 setns, 1
w2 setp, 1
w2 setnp, 1
w2 setl, 1
w2 setnl, 1
w2 setle, 1
w2 setnle, 1

mov rax, 0
w3 bt, 2, ax
w3 bt, 4, eax
w3 bt, 8, rax

w4 shld, 2, ax, 1
w4 shld, 4, eax, 1
w4 shld, 8, rax, 1

mov cl, 1
w4 shld, 2, ax, cl
w4 shld, 4, eax, cl
w4 shld, 8, rax, cl

mov rax, 0
w3 bts, 2, ax
w3 bts, 4, eax
w3 bts, 8, rax

w4 shrd, 2, ax, 1
w4 shrd, 4, eax, 1
w4 shrd, 8, rax, 1

mov cl, 1
w4 shrd, 2, ax, cl
w4 shrd, 4, eax, cl
w4 shld, 8, rax, cl

r3 imul, 8, rax
r3 imul, 4, eax
r3 imul, 2, ax

w3 cmpxchg, 8, rax
w3 cmpxchg, 4, eax
w3 cmpxchg, 2, ax
w3 cmpxchg, 1, al

mov rax, 0
w3 btr, 2, ax
w3 btr, 4, eax
w3 btr, 8, rax

; MOVZX is a bit special
movzx rax, byte [r15 - 1]
movzx rax, byte [r14]
movzx rax, word [r15 - 2]
movzx rax, word [r14]

mov rax, 0
w3 btc, 2, ax
w3 btc, 4, eax
w3 btc, 8, rax

r3 bsf, 2, ax
r3 bsf, 4, eax
r3 bsf, 8, rax

r3 bsr, 2, ax
r3 bsr, 4, eax
r3 bsr, 8, rax

; MOVSX is a bit special
movsx rax, byte [r15 - 1]
movsx rax, byte [r14]
movsx rax, word [r15 - 2]
movsx rax, word [r14]

w3 xadd, 1, al
w3 xadd, 2, ax
w3 xadd, 4, eax
w3 xadd, 8, rax

w3 lock xadd, 1, al
w3 lock xadd, 2, ax
w3 lock xadd, 4, eax
w3 lock xadd, 8, rax


r4 cmpps, 16, xmm0, 0
r4 cmpps, 16, xmm0, 1
r4 cmpps, 16, xmm0, 2
r4 cmpps, 16, xmm0, 3
r4 cmpps, 16, xmm0, 4
r4 cmpps, 16, xmm0, 5
r4 cmpps, 16, xmm0, 6
r4 cmpps, 16, xmm0, 7

w3 movnti, 4, eax
w3 movnti, 8, rax

r4 pinsrw, 2, xmm0, 0
r4 shufps, 16, xmm0, 0

r3 psrlw, 8, mm0
r3 psrld, 8, mm0
r3 psrlq, 8, mm0
r3 paddq, 8, mm0
r3 pmullw, 8, mm0
r3 psubusb, 8, mm0
r3 psubusw, 8, mm0
r3 pminub, 8, mm0
r3 pand, 8, mm0
r3 paddusb, 8, mm0
r3 paddusw, 8, mm0
r3 pmaxub, 8, mm0
r3 pandn, 8, mm0
r3 pavgb, 8, mm0
r3 psraw, 8, mm0
r3 psrad, 8, mm0
r3 pavgw, 8, mm0
r3 pmulhuw, 8, mm0
r3 pmulhw, 8, mm0

w3 movntq, 8, mm0


r3 psubsb, 8, mm0
r3 psubsw, 8, mm0
r3 pminsw, 8, mm0
r3 por, 8, mm0
r3 paddsb, 8, mm0
r3 paddsw, 8, mm0
r3 pmaxsw, 8, mm0
r3 pxor, 8, mm0
r3 psllw, 8, mm0
r3 pslld, 8, mm0
r3 psllq, 8, mm0
r3 pmuludq, 8, mm0
r3 pmaddwd, 8, mm0
r3 psadbw, 8, mm0
r3 psubb, 8, mm0
r3 psubw, 8, mm0
r3 psubd, 8, mm0
r3 psubq, 8, mm0
r3 paddb, 8, mm0
r3 paddw, 8, mm0
r3 paddd, 8, mm0

; Done
mov rax, 1
hlt
