%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RBX": "0x4142434445464748",
    "RCX": "0x0000000000000056",
    "RDX": "0x4142434445464748",
    "RSI": "0x0000000000004748",
    "RDI": "0x4142434445464748",
    "RSP": "0x0000000045464748",
    "RBP": "0x6162636465666768",
    "R8": "0x4142434445464748",
    "R9": "0x4142434445464748",
    "R10": "0x0000000000000056",
    "R11": "0x4142434445464748",
    "R12": "0x4142434445464748",
    "R13": "0x0000000000004748",
    "R14": "0x0000000045464748",
    "R15": "0x6162636465666768",
    "XMM0": ["0x5152535455565758", "0x4142434445464748"],
    "XMM1": ["0x4142434445464748", "0x6162636465666768"],
    "XMM2": ["0x5152535455564858", "0x6162636465666768"],
    "XMM3": ["0x5152535455565758", "0x4142434445464748"],
    "XMM4": ["0x4142434445464748", "0x6162636465666768"],
    "XMM5": ["0x4546474855565758", "0x6162636465666768"],
    "XMM6": ["0x5152535455565758", "0x4142434445464748"],
    "XMM7": ["0x5152535447485758", "0x6162636465666768"],
    "XMM8": ["0x5152535455565758", "0x4142434445464748"],
    "XMM9": ["0x4142434445464748", "0x4142434445464748"],
    "XMM10": ["0x5152535455564858", "0x6162636465666768"],
    "XMM11": ["0x5152535455565758", "0x4142434445464748"],
    "XMM12": ["0x4142434445464748", "0x4142434445464748"],
    "XMM13": ["0x4546474855565758", "0x6162636465666768"],
    "XMM14": ["0x5152535455565758", "0x4142434445464748"],
    "XMM15": ["0x5152535447485758", "0x6162636465666768"]
  },
  "HostFeatures": ["AVX"]
}
%endif

; FEX-Emu had a bug where TLS vector element loadstores weren't correctly prefixing the segment on the address.
; This caused a crash in the game Halls of Torment (steamid 2218750) where it had some TLS vector data loaded with movhps.
; This tests all the vector element loadstores that FEX had missed.

; Setup TLS segment
mov rax, 0xe000_0000
wrgsbase rax

movups xmm0, [rel .data_setup]
movups xmm1, [rel .data_setup]
movups xmm2, [rel .data_setup]
movups xmm3, [rel .data_setup]
movups xmm4, [rel .data_setup]
movups xmm5, [rel .data_setup]
movups xmm6, [rel .data_setup]
movups xmm7, [rel .data_setup]
movups xmm8, [rel .data_setup]
movups xmm9, [rel .data_setup]
movups xmm10, [rel .data_setup]
movups xmm11, [rel .data_setup]
movups xmm12, [rel .data_setup]
movups xmm13, [rel .data_setup]
movups xmm14, [rel .data_setup]
movups xmm15, [rel .data_setup]

mov rax, [rel .data]
mov [gs:0], rax

jmp .test
.test:

; SSE loads
movhps xmm0, [gs:0]
movlps xmm1, [gs:0]
pinsrb xmm2, [gs:0], 1
movhpd xmm3, [gs:0]
movlpd xmm4, [gs:0]
pinsrd xmm5, [gs:0], 1
pinsrq xmm6, [gs:0], 1
pinsrw xmm7, [gs:0], 1

; AVX loads
vmovhps xmm8, xmm0, [gs:0]
vmovlps xmm9, xmm0, [gs:0]
vpinsrb xmm10, [gs:0], 1
vmovhpd xmm11, xmm0, [gs:0]
vmovlpd xmm12, xmm0, [gs:0]
vpinsrd xmm13, [gs:0], 1
vpinsrq xmm14, [gs:0], 1
vpinsrw xmm15, [gs:0], 1

; SSE stores
movhps [gs:(0 * 8)], xmm0
movlps [gs:(1 * 8)], xmm1
pextrb [gs:(2 * 8)], xmm2, 2
movhpd [gs:(3 * 8)], xmm3
movlpd [gs:(4 * 8)], xmm4
pextrw [gs:(5 * 8)], xmm5, 2
pextrd [gs:(6 * 8)], xmm6, 2
pextrq [gs:(7 * 8)], xmm7, 1

; AVX stores
vmovhps [gs:(8 * 8)], xmm0
vmovlps [gs:(9 * 8)], xmm1
vpextrb [gs:(10 * 8)], xmm2, 2
vmovhpd [gs:(11 * 8)], xmm3
vmovlpd [gs:(12 * 8)], xmm4
vpextrw [gs:(13 * 8)], xmm5, 2
vpextrd [gs:(14 * 8)], xmm6, 2
vpextrq [gs:(15 * 8)], xmm7, 1

; Load the results back in to GPRs
mov rax, [gs:(0 * 8)]
mov rbx, [gs:(1 * 8)]
mov rcx, [gs:(2 * 8)]
mov rdx, [gs:(3 * 8)]
mov rdi, [gs:(4 * 8)]
mov rsi, [gs:(5 * 8)]
mov rsp, [gs:(6 * 8)]
mov rbp, [gs:(7 * 8)]
mov r8, [gs:(8 * 8)]
mov r9, [gs:(9 * 8)]
mov r10, [gs:(10 * 8)]
mov r11, [gs:(11 * 8)]
mov r12, [gs:(12 * 8)]
mov r13, [gs:(13 * 8)]
mov r14, [gs:(14 * 8)]
mov r15, [gs:(15 * 8)]

hlt

.data:
dq 0x4142434445464748

.data_setup:
dq 0x5152535455565758, 0x6162636465666768

.data_result:
dq 0, 0, 0, 0, 0, 0, 0, 0
