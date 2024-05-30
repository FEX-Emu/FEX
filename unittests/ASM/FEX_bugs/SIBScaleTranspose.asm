%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x5152535455565758",
    "RBX": "0x5152535455565758",
    "RCX": "0x5152535455565758",
    "RDX": "0x5152535455565758",
    "RDI": "0x5152535455565758",

    "XMM0": ["0x5152535455565758", "0x0"],
    "XMM1": ["0x5152535455565758", "0x0"],
    "XMM2": ["0x5152535455565758", "0x0"],
    "XMM3": ["0x5152535455565758", "0x0"],
    "XMM4": ["0x5152535455565758", "0x0"],

    "MM0": "0x5152535455565758",
    "MM1": "0x5152535455565758",
    "MM2": "0x5152535455565758",
    "MM3": "0x5152535455565758",
    "MM4": "0x5152535455565758"
  },
  "MemoryRegions": {
    "0x00000000a0000000": "4096",
    "0x0000000110000000": "4096"
  },
  "MemoryData": {
    "0x00000000a0000000": "0x4142434445464748",
    "0x0000000110000000": "0x5152535455565758"
  }
}
%endif

; FEX had a bug in its const-prop pass where x86 SIB scale would accidentally transpose the register that was scaling with the base.
; This test explicitly tests SIB in a way that a transpose would load data from the wrong address.
; Basic layout is [r14 + (r15 * 8)]

; r14 will be the base
mov r14, 0x1000_0000
; r15 will be the index
mov r15, 0x2000_0000

; Correct transpose will be at 0x0000000110000000
; Incorrect transpose will be at 0x00000000a0000000

; Break the block
jmp .test
.test:

; Basic GPR SIB test
mov rax, [r14 + (r15 * 8)]

; Basic Vector SIB test
movq xmm0, [r14 + (r15 * 8)]

; Basic MMX SIB test
movq mm0, [r14 + (r15 * 8)]

; Break the block now
jmp .test2
.test2:

; FEX GPR/XMM LoadMem const prop might only happen with disjoint add + mul so check this
; Need to be able to const-prop the multiply
imul r13, r15, 8

; Test base + offset transposed both ways, for all three types
mov rbx, [r14 + r13]
mov rcx, [r13 + r14]

movq xmm1, [r14 + r13]
movq xmm2, [r13 + r14]

movq mm1, [r14 + r13]
movq mm2, [r13 + r14]

; Break the block now
jmp .test3
.test3:

; FEX GPR/XMM LoadMem const prop might only happen with disjoint add + lshl so check this
; Need to be able to const-prop the lshl
mov r13, r15
shl r13, 3

; Test base + offset transposed both ways, for all three types
mov rdx, [r14 + r13]
mov rdi, [r13 + r14]

movq xmm3, [r14 + r13]
movq xmm4, [r13 + r14]

movq mm3, [r14 + r13]
movq mm4, [r13 + r14]

hlt
