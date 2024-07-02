%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x0000000000000078", "0x0000000000000077"],
    "XMM1": ["0x0000000000000078", "0x0000000000000077"],
    "XMM2": ["0x0000000000000078", "0x0000000000000077"],
    "XMM3": ["0x0000000000000078", "0x7800000000000077"],
    "XMM4": ["0x0000000000000078", "0"],
    "XMM5": ["0x0000000000000078", "0"]
  }
}
%endif

; FEX-Emu had a bug with vector loadstore instructions where 16-bit and 8-bit vector loadstores with reg+reg source would assert in the code emitter.
; This affected both vector loads and stores. SSE 8-bit and 16-bit are quite uncommon so this isn't encountered frequently.
; Tests a few different instructions that access 16-bit and 8-bit with loads and stores.
lea rax, [rel .data]
lea rbx, [rel .data_temp]
mov rcx, 0
jmp .test
.test:

; 16-bit loads
pmovzxbq xmm0, [rax + rcx]
pmovzxbq xmm1, [rax + rcx*2]
pmovzxbq xmm2, [rax + rcx*4]
pmovzxbq xmm3, [rax + rcx*8]

; 8-bit load
pinsrb xmm3, [rax + rcx], 1111b

; 8-bit store
pextrb [rbx + rcx], xmm0, 0

; Load the result back
movaps xmm4, [rbx + rcx]

; 16-bit store
pextrb [rbx + rcx], xmm0, 0

; Load the result back
movaps xmm5, [rbx + rcx]

hlt
align 32
.data:
dq 0x7172737475767778
dq 0x4142434445464748

.data_temp:
dq 0,0
