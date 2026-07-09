%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xDEADBEEF11223344",
    "RBX": "0x0000000011223344",
    "RCX": "0xFEEDFEED11223344",
     "XMM0": [
      "0x0000000011223344",
      "0x0000000000000000",
      "0xDEADBEEFCAFEBABE",
      "0xDEADBEEFCAFEBABE"
    ],
     "XMM3": [
      "0x0000000011223344",
      "0x0000000000000000",
      "0xDEADBEEFCAFEBABE",
      "0xDEADBEEFCAFEBABE"
    ]
  }
}
%endif

lea rdx, [rel .data]
vmovdqa ymm0, [rdx]


;; movd XMM, GREG
mov rax, 0xDEADBEEF11223344

o16 movd xmm0, eax


;; movd GREG, XMM
vmovdqa ymm1, [rdx + 32]
mov rbx, 0xDEADBEEFCAFEBABE

o16 movd ebx, xmm1


;; movd MEM, XMM
vmovdqa ymm2, [rdx + 32]
o16 movd dword [rdx + 64], xmm2

mov rcx, qword [rdx + 64]


;; movd XMM, MEM
vmovdqa ymm3, [rdx]

o16 movd xmm3, dword [rdx + 72]


hlt


align 4096
.data:
; YMM0, YMM3 init
dq 0xDEADBEEFCAFEBABE, 0xDEADBEEFCAFEBABE, 0xDEADBEEFCAFEBABE, 0xDEADBEEFCAFEBABE
; YMM1, YMM2 init
dq 0xDEADBEEF11223344, 0xDEADBEEFCAFEBABE, 0xDEADBEEFCAFEBABE, 0xDEADBEEFCAFEBABE
; movd MEM, XMM result
dq 0xFEEDFEEDFEEDFEED
dq 0xDEADBEEF11223344