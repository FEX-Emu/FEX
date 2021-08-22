%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1111111111111111",
    "RBX": "0x2222222222222222",
    "RCX": "0x3333333333333333",
    "RDX": "0x4444444444444444",
    "RSI": "0x5555555555555555",
    "RDI": "0x6666666666666666",
    "MM0": "0x1112131415161718",
    "MM1": "0x2122232425262728",
    "MM2": "0x3132333435363738",
    "MM3": "0x4142434445464748",
    "MM4": "0x5152535455565758",
    "MM5": "0x6162636465666768",
    "MM6": "0x7172737475767778",
    "MM7": "0x8182838485868788",
    "XMM0":  ["0x1112131415161718", "0x0"],
    "XMM1":  ["0x2122232425262728", "0x0"],
    "XMM2":  ["0x3132333435363738", "0x0"],
    "XMM3":  ["0x4142434445464748", "0x0"],
    "XMM4":  ["0x5152535455565758", "0x0"],
    "XMM5":  ["0x6162636465666768", "0x0"],
    "XMM6":  ["0x7172737475767778", "0x0"],
    "XMM7":  ["0x8182838485868788", "0x0"],
    "XMM8":  ["0xccc2c3c4c5c6c7c8", "0x0"],
    "XMM9":  ["0xa1aaa3a4a5a6a7a8", "0x0"],
    "XMM10": ["0xf1f2fff4f5f6f7f8", "0x0"],
    "XMM11": ["0xe1e2e3eee5e6e7e8", "0x0"],
    "XMM12": ["0xd1d2d3d4ddd6d7d8", "0x0"],
    "XMM13": ["0xc1c2c3c4c5ccc7c8", "0x0"],
    "XMM14": ["0xb1b2b3b4b5b6bbb8", "0x0"],
    "XMM15": ["0xa1a2a3a4a5a6a7aa", "0x0"]
  }
}
%endif

mov rsp, 0xe0000000

; Set up MMX state
mov rax, 0x1112131415161718
movd mm0, rax
mov rax, 0x2122232425262728
movd mm1, rax
mov rax, 0x3132333435363738
movd mm2, rax
mov rax, 0x4142434445464748
movd mm3, rax
mov rax, 0x5152535455565758
movd mm4, rax
mov rax, 0x6162636465666768
movd mm5, rax
mov rax, 0x7172737475767778
movd mm6, rax
mov rax, 0x8182838485868788
movd mm7, rax

; Setup XMM state
mov rax, 0x1112131415161718
movq xmm0, rax
mov rax, 0x2122232425262728
movq xmm1, rax
mov rax, 0x3132333435363738
movq xmm2, rax
mov rax, 0x4142434445464748
movq xmm3, rax
mov rax, 0x5152535455565758
movq xmm4, rax
mov rax, 0x6162636465666768
movq xmm5, rax
mov rax, 0x7172737475767778
movq xmm6, rax
mov rax, 0x8182838485868788
movq xmm7, rax
mov rax, 0xccc2c3c4c5c6c7c8
movq xmm8, rax
mov rax, 0xa1aaa3a4a5a6a7a8
movq xmm9, rax
mov rax, 0xf1f2fff4f5f6f7f8
movq xmm10, rax
mov rax, 0xe1e2e3eee5e6e7e8
movq xmm11, rax
mov rax, 0xd1d2d3d4ddd6d7d8
movq xmm12, rax
mov rax, 0xc1c2c3c4c5ccc7c8
movq xmm13, rax
mov rax, 0xb1b2b3b4b5b6bbb8
movq xmm14, rax
mov rax, 0xa1a2a3a4a5a6a7aa
movq xmm15, rax

; Corrupt state and see what it stores
mov eax, 0x41424344

; Overwrite header
mov dword [rsp + 0], eax
; Overwrite the mm state
mov rax, -1
mov qword [rsp + 32 + 8 * 0], rax
mov qword [rsp + 32 + 8 * 1], rax
mov qword [rsp + 32 + 8 * 2], rax
mov qword [rsp + 32 + 8 * 3], rax
mov qword [rsp + 32 + 8 * 4], rax
mov qword [rsp + 32 + 8 * 5], rax
mov qword [rsp + 32 + 8 * 6], rax
mov qword [rsp + 32 + 8 * 7], rax

; Overwrite the xmm state
mov qword [rsp + 160 + 8 * 0], rax
mov qword [rsp + 160 + 8 * 1], rax
mov qword [rsp + 160 + 8 * 2], rax
mov qword [rsp + 160 + 8 * 3], rax
mov qword [rsp + 160 + 8 * 4], rax
mov qword [rsp + 160 + 8 * 5], rax
mov qword [rsp + 160 + 8 * 6], rax
mov qword [rsp + 160 + 8 * 7], rax
mov qword [rsp + 160 + 8 * 8], rax
mov qword [rsp + 160 + 8 * 9], rax
mov qword [rsp + 160 + 8 * 10], rax
mov qword [rsp + 160 + 8 * 11], rax
mov qword [rsp + 160 + 8 * 12], rax
mov qword [rsp + 160 + 8 * 13], rax
mov qword [rsp + 160 + 8 * 14], rax
mov qword [rsp + 160 + 8 * 15], rax
mov qword [rsp + 160 + 8 * 16], rax
mov qword [rsp + 160 + 8 * 17], rax
mov qword [rsp + 160 + 8 * 18], rax
mov qword [rsp + 160 + 8 * 19], rax
mov qword [rsp + 160 + 8 * 20], rax
mov qword [rsp + 160 + 8 * 21], rax
mov qword [rsp + 160 + 8 * 22], rax
mov qword [rsp + 160 + 8 * 23], rax
mov qword [rsp + 160 + 8 * 24], rax
mov qword [rsp + 160 + 8 * 25], rax
mov qword [rsp + 160 + 8 * 26], rax
mov qword [rsp + 160 + 8 * 27], rax
mov qword [rsp + 160 + 8 * 28], rax
mov qword [rsp + 160 + 8 * 29], rax
mov qword [rsp + 160 + 8 * 30], rax
mov qword [rsp + 160 + 8 * 31], rax

; Overwrite the three reserved 16byte elements
mov qword [rsp + 416 + 8 * 0], rax
mov qword [rsp + 416 + 8 * 1], rax
mov qword [rsp + 416 + 8 * 2], rax
mov qword [rsp + 416 + 8 * 3], rax
mov qword [rsp + 416 + 8 * 4], rax
mov qword [rsp + 416 + 8 * 5], rax

; Overwrite the three 16byte "available" slots
mov rax, 0x1111111111111111
mov qword [rsp + 464 + 8 * 0], rax
mov rax, 0x2222222222222222
mov qword [rsp + 464 + 8 * 1], rax
mov rax, 0x3333333333333333
mov qword [rsp + 464 + 8 * 2], rax
mov rax, 0x4444444444444444
mov qword [rsp + 464 + 8 * 3], rax
mov rax, 0x5555555555555555
mov qword [rsp + 464 + 8 * 4], rax
mov rax, 0x6666666666666666
mov qword [rsp + 464 + 8 * 5], rax

; Now save our state
fxsave [rsp]

; Corrupt MMX And XMM state
mov rax, -1
movd mm0, rax
movd mm1, rax
movd mm2, rax
movd mm3, rax
movd mm4, rax
movd mm5, rax
movd mm6, rax
movd mm7, rax

; Setup XMM state
movq xmm0, rax
movq xmm1, rax
movq xmm2, rax
movq xmm3, rax
movq xmm4, rax
movq xmm5, rax
movq xmm6, rax
movq xmm7, rax
movq xmm8, rax
movq xmm9, rax
movq xmm10, rax
movq xmm11, rax
movq xmm12, rax
movq xmm13, rax
movq xmm14, rax
movq xmm15, rax

; Now reload the state we just saved
fxrstor [rsp]

; Load the three 16bytes of "available" slots to make sure it wasn't overwritten
; Reserved can be overwritten regardless
mov rax, qword [rsp + 464 + 8 * 0]
mov rbx, qword [rsp + 464 + 8 * 1]
mov rcx, qword [rsp + 464 + 8 * 2]
mov rdx, qword [rsp + 464 + 8 * 3]
mov rsi, qword [rsp + 464 + 8 * 4]
mov rdi, qword [rsp + 464 + 8 * 5]

hlt
