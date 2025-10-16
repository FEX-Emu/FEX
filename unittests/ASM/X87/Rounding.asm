%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x212121211121",
    "RCX": "0xfffefffeffffffff",
    "RDX": "0xfffffffeffffffff",
    "RSI": "0xfffefffeffffffff"
  }
}
%endif

; Rounding tests to ensure rounding modes are actually working

;; Mid-point
finit
fld qword [rel midpoint]

; Default rounding is 00 - round to nearest
fist dword [rel tmp]
mov rbx, qword [rel tmp]
shl rbx, 4

; Round down - 01
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0400
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
or rbx, qword [rel tmp]
shl rbx, 4

; Round up - 10
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0800
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
or rbx, qword [rel tmp]
shl rbx, 4

; Round toward zero - 11
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0c00
mov word [rel tmp], ax
fldcw word [rel tmp]

fistp dword [rel tmp]
or rbx, qword [rel tmp]
shl rbx, 4

;; Slightly above midpoint
finit
fld qword [rel samidpoint]

; Default rounding is 00 - round to nearest
fist dword [rel tmp]
or rbx, qword [rel tmp]
shl rbx, 4

; Round down - 01
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0400
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
or rbx, qword [rel tmp]
shl rbx, 4

; Round up - 10
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0800
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
or rbx, qword [rel tmp]
shl rbx, 4

; Round toward zero - 11
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0c00
mov word [rel tmp], ax
fldcw word [rel tmp]

fistp dword [rel tmp]
or rbx, qword [rel tmp]
shl rbx, 4

;; Slightly below midpoint
finit
fld qword [rel sbmidpoint]

; Default rounding is 00 - round to nearest
fist dword [rel tmp]
or rbx, qword [rel tmp]
shl rbx, 4

; Round down - 01
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0400
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
or rbx, qword [rel tmp]
shl rbx, 4

; Round up - 10
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0800
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
or rbx, qword [rel tmp]
shl rbx, 4

; Round toward zero - 11
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0c00
mov word [rel tmp], ax
fldcw word [rel tmp]

fistp dword [rel tmp]
or rbx, qword [rel tmp]


;;; Negative tests
;; Mid-point
finit
fld qword [rel nmidpoint]

; Default rounding is 00 - round to nearest
fist word [rel tmp]
mov ax, word [rel tmp]
or rcx, rax 
shl rcx, 16

; Round down - 01
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0400
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
mov ax, word [rel tmp]
or rcx, rax
shl rcx, 16

; Round up - 10
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0800
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
mov ax, word [rel tmp]
or rcx, rax
shl rcx, 16

; Round toward zero - 11
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0c00
mov word [rel tmp], ax
fldcw word [rel tmp]

fistp dword [rel tmp]
mov ax, word [rel tmp]
or rcx, rax

;; Slightly above midpoint
finit
fld qword [rel nsamidpoint]

; Default rounding is 00 - round to nearest
fist dword [rel tmp]
mov dx, word [rel tmp]
shl rdx, 16

; Round down - 01
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0400
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
mov ax, word [rel tmp]
or rdx, rax
shl rdx, 16

; Round up - 10
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0800
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
mov ax, word [rel tmp]
or rdx, rax
shl rdx, 16

; Round toward zero - 11
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0c00
mov word [rel tmp], ax
fldcw word [rel tmp]

fistp dword [rel tmp]
mov ax, word [rel tmp]
or rdx, rax

;; Slightly below midpoint
finit
fld qword [rel nsbmidpoint]

; Default rounding is 00 - round to nearest
fist dword [rel tmp]
mov si, word [rel tmp]
shl rsi, 16

; Round down - 01
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0400
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
mov ax, word [rel tmp]
or rsi, rax
shl rsi, 16

; Round up - 10
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0800
mov word [rel tmp], ax
fldcw word [rel tmp]

fist dword [rel tmp]
mov ax, word [rel tmp]
or rsi, rax
shl rsi, 16

; Round toward zero - 11
fstcw word [rel tmp]
movzx rax, word [rel tmp]
and rax, 0xf3ff
or rax, 0x0c00
mov word [rel tmp], ax
fldcw word [rel tmp]

fistp dword [rel tmp]
mov ax, word [rel tmp]
or rsi, rax

hlt

align 4096
midpoint:
  dq 1.5
samidpoint:
  dq 1.50001
sbmidpoint:
  dq 1.49999
nmidpoint:
  dq -1.5
nsamidpoint:
  dq -1.49999
nsbmidpoint:
  dq -1.50001
tmp: dq 0
