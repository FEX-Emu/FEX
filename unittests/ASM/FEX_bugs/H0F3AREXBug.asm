%ifdef CONFIG
{}
%endif

; FEX-Emu had a bug in decoding the H0F3A instruction table.
; It would accidentally require REX.W to not be set on the suite of instructions that ignore the flag.
; This just executes all instructions from H0F3A that ignore the REX.W flag, to ensure it decodes.

o64 palignr mm0, mm1, 0
o64 roundps xmm0, xmm1, 0
o64 roundpd xmm0, xmm1, 0
o64 roundss xmm0, xmm1, 0
o64 roundsd xmm0, xmm1, 0
o64 blendps xmm0, xmm1, 0
o64 blendpd xmm0, xmm1, 0
o64 palignr xmm0, xmm1, 0
o64 pextrb eax, xmm0, 0
o64 pextrw eax, xmm0, 0
o64 extractps eax, xmm0, 0
o64 extractps eax, xmm0, 0
o64 pinsrb xmm0, eax, 0
o64 insertps xmm0, xmm1, 0
o64 dpps xmm0, xmm1, 0
o64 dppd xmm0, xmm1, 0
o64 mpsadbw xmm0, xmm1, 0
o64 pclmulqdq xmm0, xmm1, 0
o64 pcmpestrm xmm0, xmm1, 0
o64 pcmpestri xmm0, xmm1, 0
o64 pcmpistrm xmm0, xmm1, 0
o64 pcmpistri xmm0, xmm1, 0
o64 sha1rnds4 xmm0, xmm1, 0
o64 aeskeygenassist xmm0, xmm1, 0

hlt
