%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0",
    "RBX": "0x0",
    "RCX": "0x8000000000000000",
    "RDX": "0x3FFF",
    "R8": "0xc90fdaa22168c235",
    "R9": "0x4000",
    "R10": "0xc90fdaa22168c235",
    "R11": "0xFFFF"
  }
}
%endif

section .bss
  x87env: resb 108

section .text
global _start
; Checks that after moving from X87 to MMX States, the
; values are correct and that MMX register writes, puts the top 16 bits as
; all 1s.
_start:
finit ; enters x87 state

fldpi ; goes in mm7
fld1  ; goes in mm6

movq mm5, mm7 ; enters mmx state, so 1 is now in st6 and pi in st7, while st5 has a broken pi.
o32 fnsave [rel x87env]

; Top into eax
mov eax, dword [rel x87env + 4]
and eax, 0x3800
shr eax, 11 ; top in eax

; Tag into ebx
mov bx, word [rel x87env + 8]

; st6 is 1
mov rcx, qword [rel x87env + 88]
mov dx, word [rel x87env + 96]

; st7 is pi
mov r8, qword [rel x87env + 98]
mov r9w, word [rel x87env + 106]

; st5 is broken pi
mov r10, qword [rel x87env + 78]
mov r11w, word [rel x87env + 86]

hlt
