%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x5858585858585858", "0x5858585858585858"],
    "XMM1": ["0x0", "0x0"]
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

movaps xmm0, [rdx + 8 * 0]
movaps xmm1, [rdx + 8 * 0]

lea rdx, [rel .data]

pshufb xmm0, [rdx + 8 * 0]
pshufb xmm1, [rdx + 8 * 2]

hlt

align 8
.data:
; Test bits with trash data in reserved bits to ensure it is ignored
; Select single element
dq 0x7878787878787878
dq 0x7878787878787878
; Clear element
dq 0xF0F0F0F0F0F0F0F0
dq 0xF0F0F0F0F0F0F0F0

