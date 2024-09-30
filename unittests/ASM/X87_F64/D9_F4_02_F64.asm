%ifdef CONFIG
{
  "RegData": {
    "RAX":  ["0xFFF0000000000000"],
    "RBX":  ["0x0000000000000000"],
    "RCX":  ["0xFFF0000000000000"],
    "RDX":  ["0x8000000000000000"]
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

; Instead of checking MMX registers, 
; move results to general purpose registers and check them there
; so that hostrunner tests work properly.

section .data
    nzer: dq -0.0

section .bss
    expz: resq 1
    sigz: resq 1
    expnz: resq 1
    signz: resq 1

section .text
global _start
_start:
finit
fldz
fxtract
fstp qword [rel sigz]
fstp qword [rel expz]

lea rdx, [rel nzer]
fld qword [rdx]
fxtract
fstp qword [rel signz]
fstp qword [rel expnz]

mov rax, [rel expz]
mov rbx, [rel sigz]
mov rcx, [rel expnz]
mov rdx, [rel signz]

hlt

