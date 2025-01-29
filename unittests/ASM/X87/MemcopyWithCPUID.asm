%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x8000000000000000",
    "RCX": "0x3fff"
  }
}
%endif

; Related to #4274 - ensures that if cpuid clobbers the predicate register,
; we reset the predicate cache.

section .data
align 8

data:
  dt 1.0

section .bss
align 8

data2:
  resb 10

section .text
lea r8, [rel data]
fld tword [r8]

mov rax, 0x0
cpuid ; Will this instruction clobber the predicate register?

fstp tword [rel data2]

mov rbx, [rel data2]
mov rcx, [rel data2+8]
hlt
