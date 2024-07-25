%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xfffefffe",
    "RBX": "0xffffffff",
    "RCX": "0xfffffffe",
    "RDX": "0xffffffff",
    "RSI": "0xfffefffe",
    "RDI": "0xffffffff"
  },
  "MemoryRegions": {
    "0xf0000000": "4096"
  },
  "Mode": "32BIT"
}
%endif


section .data
align 4
nmidpoint:
  dd -1.5
nsamidpoint:
  dd -1.49999
nsbmidpoint:
  dd -1.50001

section .bss
align 4
tmp resd 1

section .text

; Rounding tests to ensure rounding modes are actually working
;;; Negative tests
;; Mid-point
finit
fld dword [rel nmidpoint]

; Default rounding is 00 - round to nearest
fist word [rel tmp]
mov di, word [rel tmp]
mov eax, edi
shl eax, 16

; Round down - 01
fstcw word [rel tmp]
movzx edi, word [rel tmp]
and edi, 0xf3ff
or edi, 0x0400
mov word [rel tmp], di
fldcw word [rel tmp]

fist dword [rel tmp]
mov di, word [rel tmp]
or eax, edi

; Round up - 10
fstcw word [rel tmp]
movzx edi, word [rel tmp]
and edi, 0xf3ff
or edi, 0x0800
mov word [rel tmp], di
fldcw word [rel tmp]

fist dword [rel tmp]
mov di, word [rel tmp]
mov ebx, edi
shl ebx, 16

; Round toward zero - 11
fstcw word [rel tmp]
movzx edi, word [rel tmp]
and edi, 0xf3ff
or edi, 0x0c00
mov word [rel tmp], di
fldcw word [rel tmp]

fistp dword [rel tmp]
mov di, word [rel tmp]
or ebx, edi

;; Slightly above midpoint
finit
fld dword [rel nsamidpoint]

; Default rounding is 00 - round to nearest
fist dword [rel tmp]
mov di, word [rel tmp]
mov ecx, edi
shl ecx, 16

; Round down - 01
fstcw word [rel tmp]
movzx edi, word [rel tmp]
and edi, 0xf3ff
or edi, 0x0400
mov word [rel tmp], di
fldcw word [rel tmp]

fist dword [rel tmp]
mov di, word [rel tmp]
or ecx, edi

; Round up - 10
fstcw word [rel tmp]
movzx edi, word [rel tmp]
and edi, 0xf3ff
or edi, 0x0800
mov word [rel tmp], di
fldcw word [rel tmp]

fist dword [rel tmp]
mov di, word [rel tmp]
or edx, edi
shl edx, 16

; Round toward zero - 11
fstcw word [rel tmp]
movzx edi, word [rel tmp]
and edi, 0xf3ff
or edi, 0x0c00
mov word [rel tmp], di
fldcw word [rel tmp]

fistp dword [rel tmp]
mov di, word [rel tmp]
or edx, edi

;; Slightly below midpoint
finit
fld dword [rel nsbmidpoint]

; Default rounding is 00 - round to nearest
fist dword [rel tmp]
mov si, word [rel tmp]
shl esi, 16

; Round down - 01
fstcw word [rel tmp]
movzx edi, word [rel tmp]
and edi, 0xf3ff
or edi, 0x0400
mov word [rel tmp], di
fldcw word [rel tmp]

fist dword [rel tmp]
mov di, word [rel tmp]
or esi, edi

; Round up - 10
fstcw word [rel tmp]
movzx ebp, word [rel tmp]
and ebp, 0xf3ff
or ebp, 0x0800
mov word [rel tmp], bp
fldcw word [rel tmp]

fist dword [rel tmp]
mov di, word [rel tmp]
or edi, ebp
shl edi, 16

; Round toward zero - 11
fstcw word [rel tmp]
movzx ebp, word [rel tmp]
and ebp, 0xf3ff
or ebp, 0x0c00
mov word [rel tmp], bp
fldcw word [rel tmp]

fistp dword [rel tmp]
mov bp, word [rel tmp]
or edi, ebp

hlt
