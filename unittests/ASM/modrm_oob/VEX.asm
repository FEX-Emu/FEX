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

; VEX map 1
%macro r_avx 1
r3 %1, 16, xmm0
r3 %1, 32, ymm0
%endmacro

%macro r_avx_2_reg 1
r4 %1, 16, xmm0, xmm1
r4 %1, 32, ymm0, ymm1
%endmacro

%macro r_avx_fma 1
r4_fma %1, 16, xmm0, xmm1
r4_fma %1, 32, ymm0, ymm2
%endmacro

%macro r2_avx 2
r4 %1, 16, xmm0, %2
r4 %1, 32, ymm0, %2
%endmacro

%macro w_avx 1
w3 %1, 16, xmm0
w3 %1, 32, ymm0
%endmacro

%macro rw_avx 1
r_avx %1
w_avx %1
%endmacro

; VEX
rw_avx vmovups
rw_avx vmovupd
rw3 vmovss, 4, xmm0
rw3 vmovsd, 8, xmm0

rw3 vmovlps, 8, xmm0
rw3 vmovlpd, 8, xmm0

r_avx vmovsldup
r_avx vmovddup

r_avx vunpcklps
r_avx vunpcklpd
r_avx vunpckhps
r_avx vunpckhpd

rw3 vmovhps, 8, xmm0
rw3 vmovhpd, 8, xmm0

r_avx vmovshdup
r_avx vsqrtps
r_avx vsqrtpd

r3 vsqrtss, 4, xmm0
r3 vsqrtsd, 8, xmm0

r_avx vrsqrtps
r3 vrsqrtss, 4, xmm0

r_avx vrcpps
r3 vrcpss, 4, xmm0

r_avx vandps
r_avx vandpd
r_avx vandnps
r_avx vandnpd
r_avx vorps
r_avx vorpd
r_avx vxorps
r_avx vxorpd
r_avx vpunpcklbw
r_avx vpunpcklwd
r_avx vpunpckldq

r_avx vpacksswb
r_avx vpcmpgtb
r_avx vpcmpgtw
r_avx vpcmpgtd
r_avx vpackuswb

r2_avx vpshufd, 0
r2_avx vpshufhw, 0
r2_avx vpshuflw, 0

r_avx vpcmpeqb
r_avx vpcmpeqw
r_avx vpcmpeqd

r2_avx vcmpps, 0
r2_avx vcmpps, 1
r2_avx vcmpps, 2
r2_avx vcmpps, 3
r2_avx vcmpps, 4
r2_avx vcmpps, 5
r2_avx vcmpps, 6
r2_avx vcmpps, 7

r2_avx vcmppd, 0
r2_avx vcmppd, 1
r2_avx vcmppd, 2
r2_avx vcmppd, 3
r2_avx vcmppd, 4
r2_avx vcmppd, 5
r2_avx vcmppd, 6
r2_avx vcmppd, 7

r4 vcmpss, 4, xmm0, 0
r4 vcmpss, 4, xmm0, 1
r4 vcmpss, 4, xmm0, 2
r4 vcmpss, 4, xmm0, 3
r4 vcmpss, 4, xmm0, 4
r4 vcmpss, 4, xmm0, 5
r4 vcmpss, 4, xmm0, 6
r4 vcmpss, 4, xmm0, 7

r4 vcmpsd, 8, xmm0, 0
r4 vcmpsd, 8, xmm0, 1
r4 vcmpsd, 8, xmm0, 2
r4 vcmpsd, 8, xmm0, 3
r4 vcmpsd, 8, xmm0, 4
r4 vcmpsd, 8, xmm0, 5
r4 vcmpsd, 8, xmm0, 6
r4 vcmpsd, 8, xmm0, 7

r4 vpinsrw, 2, xmm0, 0
w4 vpextrw, 2, xmm0, 0

r2_avx vshufps, 0
r2_avx vshufpd, 0

rw_avx vmovaps
rw_avx vmovapd

r4_size vcvtsi2ss, 4, dword, xmm0
r4_size vcvtsi2ss, 8, qword, xmm0

r4_size vcvtsi2sd, 4, dword, xmm0
r4_size vcvtsi2sd, 8, qword, xmm0

w_avx vmovntps
w_avx vmovntpd

r4_size vcvttss2si, 4, dword, eax
r4_size vcvttss2si, 4, dword, rax

r4_size vcvttsd2si, 8, qword, eax
r4_size vcvttsd2si, 8, qword, rax

r4_size vcvtss2si, 4, dword, eax
r4_size vcvtss2si, 4, dword, rax

r4_size vcvtsd2si, 8, qword, eax
r4_size vcvtsd2si, 8, qword, rax

r4_size vucomiss, 4, dword, xmm0
r4_size vucomisd, 8, qword, xmm0

r4_size vcomiss, 4, dword, xmm0
r4_size vcomisd, 8, qword, xmm0

r_avx vaddps
r_avx vaddpd
r4_size vaddss, 4, dword, xmm0
r4_size vaddsd, 8, qword, xmm0

r_avx vmulps
r_avx vmulpd
r4_size vmulss, 4, dword, xmm0
r4_size vmulsd, 8, qword, xmm0

r4_size vcvtps2pd, 8, qword, xmm0
r4_size vcvtps2pd, 16, oword, ymm0

r4_size vcvtpd2ps, 16, oword, xmm0
r4_size vcvtpd2ps, 32, yword, xmm0

r4_size vcvtss2sd, 4, dword, xmm0
r4_size vcvtsd2ss, 8, qword, xmm0

r_avx vcvtdq2ps
r_avx vcvtps2dq
r_avx vcvttps2dq

r_avx vsubps
r_avx vsubpd
r4_size vsubss, 4, dword, xmm0
r4_size vsubsd, 8, qword, xmm0

r_avx vminps
r_avx vminpd
r4_size vminss, 4, dword, xmm0
r4_size vminsd, 8, qword, xmm0

r_avx vdivps
r_avx vdivpd
r4_size vdivss, 4, dword, xmm0
r4_size vdivsd, 8, qword, xmm0

r_avx vmaxps
r_avx vmaxpd
r4_size vmaxss, 4, dword, xmm0
r4_size vmaxsd, 8, qword, xmm0

r_avx vpunpckhbw
r_avx vpunpckhwd
r_avx vpunpckhdq
r_avx vpackssdw
r_avx vpunpcklqdq
r_avx vpunpckhqdq

rw3 vmovq, 8, xmm0
rw3 vmovd, 4, xmm0

r_avx vmovdqa
r_avx vmovdqu
r_avx vhaddpd
r_avx vhaddps
r_avx vhsubpd
r_avx vhsubps
r_avx vaddsubpd
r_avx vaddsubps

r_avx vpsrlw
r_avx vpsrld
r_avx vpsrlq
r_avx vpaddq
r_avx vpmullw
r_avx vpsubusb
r_avx vpsubusw
r_avx vpand
r_avx vpaddusb
r_avx vpmaxub
r_avx vpandn
r_avx vpavgb
r_avx vpsraw
r_avx vpsrad
r_avx vpavgw
r_avx vpmulhuw
r_avx vpmulhw
r4_size vcvttpd2dq, 16, oword, xmm0
r4_size vcvttpd2dq, 32, yword, xmm0
r_avx vcvtdq2pd
r4_size vcvtpd2dq, 16, oword, xmm0
r4_size vcvtpd2dq, 32, yword, xmm0
w_avx vmovntdq
r_avx vpsubsb
r_avx vpsubsw
r_avx vpminsw
r_avx vpor
r_avx vpaddsb
r_avx vpaddsw
r_avx vpmaxsw
r_avx vpxor
r_avx vlddqu
r_avx vpsllw
r_avx vpslld
r_avx vpsllq
r_avx vpmuludq
r_avx vpmaddwd
r_avx vpsadbw
r_avx vpsubb
r_avx vpsubw
r_avx vpsubd
r_avx vpsubq
r_avx vpaddb
r_avx vpaddw
r_avx vpaddd
r_avx vpaddq

; VEX Map 2
r_avx vpshufb
r_avx vphaddw
r_avx vphaddd
r_avx vphaddsw
r_avx vpmaddubsw
r_avx vphsubw
r_avx vphsubd
r_avx vphsubsw
r_avx vpsignb
r_avx vpsignw
r_avx vpsignd
r_avx vpsignd
r_avx vpmulhrsw
r_avx vpermilps
r_avx vpermilpd
r_avx vtestps
r_avx vtestpd

r4_size vcvtph2ps, 8, qword, xmm0
r4_size vcvtph2ps, 16, oword, ymm0

r3 vpermps, 32, ymm0
r_avx vptest
r4_size vbroadcastss, 4, dword, xmm0
r4_size vbroadcastss, 4, dword, ymm0

r4_size vbroadcastsd, 8, qword, ymm0
r3 vbroadcastf128, 16, ymm0

r_avx vpabsb
r_avx vpabsw
r_avx vpabsd

r4_size vpmovsxbw, 8, qword, xmm0
r4_size vpmovsxbw, 16, oword, ymm0

r4_size vpmovsxbd, 4, dword, xmm0
r4_size vpmovsxbd, 8, qword, ymm0

r4_size vpmovsxbq, 2, word, xmm0
r4_size vpmovsxbq, 4, dword, ymm0

r4_size vpmovsxwd, 8, qword, xmm0
r4_size vpmovsxwd, 16, oword, ymm0

r4_size vpmovsxwq, 4, dword, xmm0
r4_size vpmovsxwq, 8, qword, ymm0

r4_size vpmovsxdq, 8, qword, xmm0
r4_size vpmovsxdq, 16, oword, ymm0

r_avx vpmuldq
r_avx vpcmpeqq
r_avx vmovntdqa
r_avx vpackusdw

; VMASKMOVPS/PD is complex and can't be tested here.

r4_size vpmovzxbw, 8, qword, xmm0
r4_size vpmovzxbw, 16, oword, ymm0

r4_size vpmovzxbd, 4, dword, xmm0
r4_size vpmovzxbd, 8, qword, ymm0

r4_size vpmovzxbq, 2, word, xmm0
r4_size vpmovzxbq, 4, dword, ymm0

r4_size vpmovzxwd, 8, qword, xmm0
r4_size vpmovzxwd, 16, oword, ymm0

r4_size vpmovzxwq, 4, dword, xmm0
r4_size vpmovzxwq, 8, qword, ymm0

r4_size vpmovzxdq, 8, qword, xmm0
r4_size vpmovzxdq, 16, oword, ymm0

r3 vpermd, 32, ymm0
r_avx vpcmpgtq
r_avx vpminsb
r_avx vpminsd
r_avx vpminuw
r_avx vpminud
r_avx vpmaxsb
r_avx vpmaxsd
r_avx vpmaxuw
r_avx vpmaxud
r_avx vpmulld
r3 vphminposuw, 16, xmm0
r_avx vpsrlvd
r_avx vpsrlvq
r_avx vpsravd
r_avx vpsllvd
r_avx vpsllvq

r4_size vpbroadcastd, 4, dword, xmm0
r4_size vpbroadcastd, 4, dword, ymm0

r4_size vpbroadcastq, 8, qword, xmm0
r4_size vpbroadcastq, 8, qword, ymm0

r4_size vbroadcasti128, 16, oword, ymm0

; VPMASKMOVD/Q is complex and can't be tested here.
; V{P,}GATHER* is complex and can't be tested here.
r_avx_fma vfmaddsub132pd
r_avx_fma vfmsubadd132pd
r_avx_fma vfmaddsub132ps
r_avx_fma vfmsubadd132ps

r_avx_fma vfmadd132pd
r_avx_fma vfmadd132ps
r_avx_fma vfmsub132pd
r_avx_fma vfmsub132ps
r_avx_fma vfnmadd132pd
r_avx_fma vfnmadd132ps
r_avx_fma vfnmsub132pd
r_avx_fma vfnmsub132ps
r_avx_fma vfmadd213pd
r_avx_fma vfmadd213ps
r_avx_fma vfmsub213pd
r_avx_fma vfmsub213ps
r_avx_fma vfnmadd213pd
r_avx_fma vfnmadd213ps
r_avx_fma vfnmsub213pd
r_avx_fma vfnmsub213ps
r_avx_fma vfmadd231pd
r_avx_fma vfmadd231ps
r_avx_fma vfmsub231pd
r_avx_fma vfmsub231ps
r_avx_fma vfnmadd231pd
r_avx_fma vfnmadd231ps
r_avx_fma vfnmsub231pd
r_avx_fma vfnmsub231ps
r_avx_fma vfmaddsub213pd
r_avx_fma vfmaddsub213ps
r_avx_fma vfmsubadd213pd
r_avx_fma vfmsubadd213ps
r_avx_fma vfmaddsub231pd
r_avx_fma vfmaddsub231ps
r_avx_fma vfmsubadd231pd
r_avx_fma vfmsubadd231ps

r5_fma_sized vfmadd132sd, 8, qword, xmm0, xmm1
r5_fma_sized vfmadd132ss, 4, dword, xmm0, xmm1

r5_fma_sized vfmsub132sd, 8, qword, xmm0, xmm1
r5_fma_sized vfmsub132ss, 4, dword, xmm0, xmm1

r5_fma_sized vfnmadd132sd, 8, qword, xmm0, xmm1
r5_fma_sized vfnmadd132ss, 4, dword, xmm0, xmm1

r5_fma_sized vfnmsub132sd, 8, qword, xmm0, xmm1
r5_fma_sized vfnmsub132ss, 4, dword, xmm0, xmm1

r5_fma_sized vfmadd213sd, 8, qword, xmm0, xmm1
r5_fma_sized vfmadd213ss, 4, dword, xmm0, xmm1

r5_fma_sized vfmsub213sd, 8, qword, xmm0, xmm1
r5_fma_sized vfmsub213ss, 4, dword, xmm0, xmm1

r5_fma_sized vfnmadd213sd, 8, qword, xmm0, xmm1
r5_fma_sized vfnmadd213ss, 4, dword, xmm0, xmm1

r5_fma_sized vfnmsub213sd, 8, qword, xmm0, xmm1
r5_fma_sized vfnmsub213ss, 4, dword, xmm0, xmm1

r5_fma_sized vfmadd231sd, 8, qword, xmm0, xmm1
r5_fma_sized vfmadd231ss, 4, dword, xmm0, xmm1

r5_fma_sized vfmsub231sd, 8, qword, xmm0, xmm1
r5_fma_sized vfmsub231ss, 4, dword, xmm0, xmm1

r5_fma_sized vfnmadd231sd, 8, qword, xmm0, xmm1
r5_fma_sized vfnmadd231ss, 4, dword, xmm0, xmm1

r5_fma_sized vfnmsub231sd, 8, qword, xmm0, xmm1
r5_fma_sized vfnmsub231ss, 4, dword, xmm0, xmm1

r3 vaesimc, 16, xmm0
r3 vaesenc, 16, xmm0
r3 vaesenclast, 16, xmm0
r3 vaesdec, 16, xmm0
r3 vaesdeclast, 16, xmm0

r5_fma_sized andn, 4, dword, eax, ebx
r5_fma_sized andn, 8, qword, rax, rbx

; bzhi is a bit special.
bzhi eax, dword [r15 - 4], ebx
bzhi eax, dword [r14], ebx

bzhi rax, qword [r15 - 8], rbx
bzhi rax, qword [r14], rbx

r5_fma_sized pext, 4, dword, eax, ebx
r5_fma_sized pext, 8, qword, rax, rbx

r5_fma_sized pdep, 4, dword, eax, ebx
r5_fma_sized pdep, 8, qword, rax, rbx

r5_fma_sized mulx, 4, dword, eax, ebx
r5_fma_sized mulx, 8, qword, rax, rbx

; bextr is a bit special.
bextr eax, dword [r15 - 4], ebx
bextr eax, dword [r14], ebx

bextr rax, qword [r15 - 8], rbx
bextr rax, qword [r14], rbx

; shlx is a bit special.
shlx eax, dword [r15 - 4], ebx
shlx eax, dword [r14], ebx

shlx rax, qword [r15 - 8], rbx
shlx rax, qword [r14], rbx

; sarx is a bit special.
sarx eax, dword [r15 - 4], ebx
sarx eax, dword [r14], ebx

sarx rax, qword [r15 - 8], rbx
sarx rax, qword [r14], rbx

; shrx is a bit special.
shrx eax, dword [r15 - 4], ebx
shrx eax, dword [r14], ebx

shrx rax, qword [r15 - 8], rbx
shrx rax, qword [r14], rbx

; VEX Map 3
r4 vpermq, 32, ymm0, 0
r4 vpermpd, 32, ymm0, 0

r2_avx vpblendd, 0
r2_avx vpermilps, 0
r2_avx vpermilpd, 0
r4 vperm2f128, 32, ymm0, 0
r2_avx vroundps, 0
r2_avx vroundpd, 0

r4 vroundss, 4, xmm0, 0
r4 vroundsd, 8, xmm0, 0
r2_avx vblendps, 0
r2_avx vblendpd, 0
r2_avx vpblendw, 0
r2_avx vpalignr, 0

w5_size vpextrb, 1, byte, xmm0, 0
w5_size vpextrw, 2, word, xmm0, 0
w5_size vpextrd, 4, dword, xmm0, 0
w5_size vextractps, 4, dword, xmm0, 0
w5_size vpextrq, 8, qword, xmm0, 0
r4 vinsertf128, 16, ymm0, 0
w5_size vextractf128, 16, oword, ymm0, 0
w5_size vcvtps2ph, 8, qword, xmm0, 0
w5_size vcvtps2ph, 16, oword, ymm0, 0

r5_size vpinsrb, 1, byte, xmm0, 0
r5_size vinsertps, 4, dword, xmm0, 0
r5_size vpinsrd, 4, dword, xmm0, 0
r5_size vpinsrq, 8, qword, xmm0, 0
r4 vinserti128, 16, ymm0, 0
w5_size vextracti128, 16, oword, ymm0, 0

r2_avx vdpps, 0
r4 vdppd, 16, xmm0, 0
r2_avx vmpsadbw, 0
r2_avx vpclmulqdq, 0
r4 vperm2i128, 32, ymm0, 0

r_avx_2_reg vblendvps
r_avx_2_reg vblendvpd
r_avx_2_reg vpblendvb

r4 vpcmpestrm, 16, xmm0, 0
r4 vpcmpestri, 16, xmm0, 0
r4 vpcmpistrm, 16, xmm0, 0
r4 vpcmpistri, 16, xmm0, 0
r4 vaeskeygenassist, 16, xmm0, 0

r4 rorx, 4, eax, 1
r4 rorx, 8, rax, 1

; Done
mov rax, 1
hlt
