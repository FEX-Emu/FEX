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

; Secondary REP
rw3 movss, 4, xmm0
r3 movsldup, 16, xmm0
r3 movshdup, 16, xmm0

; cvtsi2ss is a bit special
cvtsi2ss xmm0, dword [r15 - 4]
cvtsi2ss xmm0, dword [r14]

cvtsi2ss xmm0, qword [r15 - 8]
cvtsi2ss xmm0, qword [r14]

w3 movntss, 4, xmm0

r3 cvttss2si, 4, eax
r3 cvttss2si, 8, rax

r3 cvtss2si, 4, eax
r3 cvtss2si, 8, rax

r3 sqrtss, 4, xmm0
r3 rsqrtss, 4, xmm0
r3 rcpss, 4, xmm0
r3 addss, 4, xmm0
r3 mulss, 4, xmm0
r3 cvtss2sd, 4, xmm0
r3 cvttps2dq, 16, xmm0
r3 subss, 4, xmm0
r3 minss, 4, xmm0
r3 divss, 4, xmm0
r3 maxss, 4, xmm0

rw3 movdqu, 16, xmm0
r4 pshufhw, 16, xmm0, 0
rw3 movq, 8, xmm0

r3 popcnt, 2, ax
r3 popcnt, 4, eax
r3 popcnt, 8, rax

r3 tzcnt, 2, ax
r3 tzcnt, 4, eax
r3 tzcnt, 8, rax

r3 lzcnt, 2, ax
r3 lzcnt, 4, eax
r3 lzcnt, 8, rax

r4 cmpss, 4, xmm0, 0
r4 cmpss, 4, xmm0, 1
r4 cmpss, 4, xmm0, 2
r4 cmpss, 4, xmm0, 3
r4 cmpss, 4, xmm0, 4
r4 cmpss, 4, xmm0, 5
r4 cmpss, 4, xmm0, 6
r4 cmpss, 4, xmm0, 7

r3 cvtdq2pd, 8, xmm0

; Done
mov rax, 1
hlt
