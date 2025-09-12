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

; H0F38
r3 pshufb, 8, mm0
r3 pshufb, 16, xmm0

r3 phaddw, 8, mm0
r3 phaddw, 16, xmm0

r3 phaddd, 8, mm0
r3 phaddd, 16, xmm0

r3 phaddsw, 8, mm0
r3 phaddsw, 16, xmm0

r3 pmaddubsw, 8, mm0
r3 pmaddubsw, 16, xmm0

r3 phsubw, 8, mm0
r3 phsubw, 16, xmm0

r3 phsubd, 8, mm0
r3 phsubd, 16, xmm0

r3 phsubsw, 8, mm0
r3 phsubsw, 16, xmm0

r3 psignb, 8, mm0
r3 psignb, 16, xmm0

r3 psignw, 8, mm0
r3 psignw, 16, xmm0

r3 psignd, 8, mm0
r3 psignd, 16, xmm0

r3 pmulhrsw, 8, mm0
r3 pmulhrsw, 16, xmm0

r3 pblendvb, 16, xmm0
r3 blendvps, 16, xmm0
r3 blendvpd, 16, xmm0
r3 ptest, 16, xmm0

r3 pabsb, 8, mm0
r3 pabsb, 16, xmm0

r3 pabsw, 8, mm0
r3 pabsw, 16, xmm0

r3 pabsd, 8, mm0
r3 pabsd, 16, xmm0

r3 pmovsxbw, 16, xmm0
r3 pmovsxbd, 16, xmm0
r3 pmovsxbq, 16, xmm0
r3 pmovsxwd, 16, xmm0
r3 pmovsxwq, 16, xmm0
r3 pmovsxdq, 16, xmm0
r3 pmuldq, 16, xmm0
r3 pcmpeqq, 16, xmm0
r3 movntdqa, 16, xmm0
r3 packusdw, 16, xmm0
r3 pmovzxbw, 16, xmm0
r3 pmovzxbd, 16, xmm0
r3 pmovzxbq, 16, xmm0
r3 pmovzxwd, 16, xmm0
r3 pmovzxwq, 16, xmm0
r3 pmovzxdq, 16, xmm0
r3 pcmpgtq, 16, xmm0
r3 pminsb, 16, xmm0
r3 pminsd, 16, xmm0
r3 pminuw, 16, xmm0
r3 pminud, 16, xmm0
r3 pmaxsb, 16, xmm0
r3 pmaxsd, 16, xmm0
r3 pmaxuw, 16, xmm0
r3 pmaxud, 16, xmm0
r3 pmulld, 16, xmm0
r3 sha1nexte, 16, xmm0
r3 sha1msg1, 16, xmm0
r3 sha1msg2, 16, xmm0
r3 sha256rnds2, 16, xmm0
r3 sha256msg1, 16, xmm0
r3 sha256msg2, 16, xmm0
r3 aesimc, 16, xmm0
r3 aesenc, 16, xmm0
r3 aesenclast, 16, xmm0
r3 aesdec, 16, xmm0
r3 aesdeclast, 16, xmm0

rw3 movbe, 2, ax
rw3 movbe, 4, eax
rw3 movbe, 8, rax

r4_size crc32, 1, byte, eax
r4_size crc32, 2, word, eax
r4_size crc32, 4, dword, eax
r4_size crc32, 8, qword, rax

r3 adcx, 4, eax
r3 adcx, 8, rax

r3 adox, 4, eax
r3 adox, 8, rax

; Done
mov rax, 1
hlt
