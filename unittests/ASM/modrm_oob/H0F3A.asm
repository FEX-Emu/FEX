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

; H0F3A
w4 pextrq, 8, xmm0, 0
r4 pinsrq, 8, xmm0, 0
w4 pextrd, 4, xmm0, 0
r4 pinsrd, 4, xmm0, 0

r4 palignr, 8, mm0, 0
r4 palignr, 16, xmm0, 0
r4 roundps, 16, xmm0, 0
r4 roundpd, 16, xmm0, 0
r4 roundpd, 16, xmm0, 0
r4 roundss, 4, xmm0, 0
r4 roundsd, 8, xmm0, 0
r4 blendps, 16, xmm0, 0
r4 blendpd, 16, xmm0, 0
r4 pblendw, 16, xmm0, 0
r4 palignr, 8, mm0, 0
r4 palignr, 16, xmm0, 0
w4 pextrb, 1, xmm0, 0
w4 pextrw, 2, xmm0, 0
w4 extractps, 4, xmm0, 0
r4 pinsrb, 1, xmm0, 0
r4 insertps, 4, xmm0, 0
r4 dpps, 16, xmm0, 0
r4 dppd, 16, xmm0, 0
r4 mpsadbw, 16, xmm0, 0
r4 pclmulqdq, 16, xmm0, 0
r4 pcmpestrm, 16, xmm0, 0
r4 pcmpestri, 16, xmm0, 0
r4 pcmpistrm, 16, xmm0, 0
r4 pcmpistri, 16, xmm0, 0
r4 sha1rnds4, 16, xmm0, 0
r4 aeskeygenassist, 16, xmm0, 0

; Done
mov rax, 1
hlt
