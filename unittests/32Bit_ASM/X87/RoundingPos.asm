%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x21212121",
    "RCX": "0x1121"
  },
  "MemoryRegions": {
    "0xf0000000": "4096"
  },
  "Mode": "32BIT"
}
%endif


section .data
align 4
midpoint:
  dd 1.5
samidpoint:
  dd 1.50001
sbmidpoint:
  dd 1.49999

section .bss
align 4
tmp resd 1

section .text
; Rounding tests to ensure rounding modes are actually working
;; Mid-point
finit
fld dword [rel midpoint]

; Default rounding is 00 - round to nearest
fist dword [rel tmp]
mov ebx, dword [rel tmp]
shl ebx, 4

; Round down - 01
fstcw word [rel tmp]
movzx eax, word [rel tmp]
and eax, 0xf3ff
or eax, 0x0400
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
or ebx, dword [rel tmp]
shl ebx, 4

; Round up - 10
fstcw word [rel tmp]
movzx eax, word [rel tmp]
and eax, 0xf3ff
or eax, 0x0800
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
or ebx, dword [rel tmp]
shl ebx, 4

; Round toward zero - 11
fstcw word [rel tmp]
movzx eax, word [rel tmp]
and eax, 0xf3ff
or eax, 0x0c00
mov word [rel tmp], ax
fldcw word [rel tmp]

fistp dword [rel tmp]
or ebx, dword [rel tmp]
shl ebx, 4

;; Slightly above midpoint
finit
fld dword [rel samidpoint]

; Default rounding is 00 - round to nearest
fist dword [rel tmp]
or ebx, dword [rel tmp]
shl ebx, 4

; Round down - 01
fstcw word [rel tmp]
movzx eax, word [rel tmp]
and eax, 0xf3ff
or eax, 0x0400
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
or ebx, dword [rel tmp]
shl ebx, 4

; Round up - 10
fstcw word [rel tmp]
movzx eax, word [rel tmp]
and eax, 0xf3ff
or eax, 0x0800
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
or ebx, dword [rel tmp]
shl ebx, 4

; Round toward zero - 11
fstcw word [rel tmp]
movzx eax, word [rel tmp]
and eax, 0xf3ff
or eax, 0x0c00
mov word [rel tmp], ax
fldcw word [rel tmp]

fistp dword [rel tmp]
or ebx, dword [rel tmp]

;; Slightly below midpoint
finit
fld dword [rel sbmidpoint]

; Default rounding is 00 - round to nearest
fist dword [rel tmp]
mov ecx, dword [rel tmp]
shl ecx, 4

; Round down - 01
fstcw word [rel tmp]
movzx eax, word [rel tmp]
and eax, 0xf3ff
or eax, 0x0400
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
or ecx, dword [rel tmp]
shl ecx, 4

; Round up - 10
fstcw word [rel tmp]
movzx eax, word [rel tmp]
and eax, 0xf3ff
or eax, 0x0800
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
or ecx, dword [rel tmp]
shl ecx, 4

; Round toward zero - 11
fstcw word [rel tmp]
movzx eax, word [rel tmp]
and eax, 0xf3ff
or eax, 0x0c00
mov word [rel tmp], ax
fldcw word [rel tmp]

fistp dword [rel tmp]
or ecx, dword [rel tmp]

hlt
