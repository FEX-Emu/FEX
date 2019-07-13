%ifdef CONFIG
{
  "Ignore": [],
  "RegData": {
    "RAX": "1",
    "RDX": "2"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rsp, 0xe8000000
mov rdi, 0xe0000000
call .simple_loop


hlt

.simple_loop:
pxor xmm0, xmm0
pxor xmm1, xmm1
mov rax, 0xffffffffffffd8f0

.loop_top:
movdqu xmm2, [rdi + rax * 4 + 0x9c40]
paddd xmm0, xmm2
movdqu xmm2, [rdi + rax * 4 + 0x9c50]
paddd xmm1, xmm2

add rax, 8
jne .loop_top

paddd xmm1, xmm0
pshufd xmm0, xmm1, 0x4e
paddd xmm0, xmm1
pshufd xmm1, xmm0, 0xe5
paddd xmm1, xmm0
movd eax, xmm1

retq
