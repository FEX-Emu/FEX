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

; Secondary REPNE
rw3 movsd, 8, xmm0
r3 movddup, 8, xmm0

; cvtsi2sd is a bit special
cvtsi2sd xmm0, dword [r15 - 4]
cvtsi2sd xmm0, dword [r14]

cvtsi2sd xmm0, qword [r15 - 8]
cvtsi2sd xmm0, qword [r14]

w3 movntsd, 8, xmm0

r3 cvttsd2si, 8, rax
r3 cvtsd2si, 8, rax

r3 sqrtsd, 8, xmm0
r3 addsd, 8, xmm0
r3 mulsd, 8, xmm0
r3 cvtsd2ss, 8, xmm0
r3 subsd, 8, xmm0
r3 minsd, 8, xmm0
r3 divsd, 8, xmm0
r3 maxsd, 8, xmm0

r4 pshuflw, 16, xmm0, 0
r3 haddps, 16, xmm0
r3 hsubps, 16, xmm0

r4 cmpsd, 8, xmm0, 0
r4 cmpsd, 8, xmm0, 1
r4 cmpsd, 8, xmm0, 2
r4 cmpsd, 8, xmm0, 3
r4 cmpsd, 8, xmm0, 4
r4 cmpsd, 8, xmm0, 5
r4 cmpsd, 8, xmm0, 6
r4 cmpsd, 8, xmm0, 7

r3 addsubps, 16, xmm0
r3 cvtpd2dq, 16, xmm0
r3 lddqu, 16, xmm0

; Done
mov rax, 1
hlt
