%ifdef CONFIG
{
  "RegData": {
    "R15": "0x00000000012a2040"
  }
}
%endif

%macro cfmerge 0

; Get CF
lahf
shr rax, 8
and rax, 1

; Merge in to results
shl r15, 1
or r15, rax

%endmacro

stc
cfmerge

; 8-bit
; Shift 1 past size - Bit Set
mov rbx, 0x800
ror bl, 9
cfmerge

; Shift 1 past size - Bit unset
mov rbx, 0x000
ror bl, 9
cfmerge

; Shift size - Bit Set
mov rbx, 0x80
ror bl, 8
cfmerge

; Shift size - Bit unset
mov rbx, 0x8000
ror bl, 8
cfmerge

; 8-bit - wrapped
; Shift 1 past size - Bit Set
mov rbx, 0x01
ror bl, 9
cfmerge

; Shift 1 past size - Bit unset
mov rbx, 0xFFF2
ror bl, 9
cfmerge

; Shift size - Bit Set
mov rbx, 0xFF
ror bl, 8
cfmerge

; Shift size - Bit unset
mov rbx, 0xFF00
ror bl, 8
cfmerge


; 16-bit
; Shift 1 past size - Bit Set
mov rbx, 0x80000
ror bx, 17
cfmerge

; Shift 1 past size - Bit unset
mov rbx, 0x00000
ror bx, 17
cfmerge

; Shift size - Bit Set
mov rbx, 0x8000
ror bx, 16
cfmerge

; Shift size - Bit unset
mov rbx, 0x80000
ror bx, 16
cfmerge

; 32-bit
; Shift 1 past size - Bit Set
mov rbx, 0x800000000
ror ebx, 33
cfmerge

; Shift 1 past size - Bit unset
mov rbx, 0x000000000
ror ebx, 33
cfmerge

; Shift size - Bit Set
mov rbx, 0x80000000
ror ebx, 32
cfmerge

; Shift size - Bit unset
mov rbx, 0x800000000
ror ebx, 32
cfmerge

; 32-bit - Wrapping
; Shift 1 past size - Bit Set
mov rbx, 0x02
ror ebx, 33
cfmerge

; Shift 1 past size - Bit unset
mov rbx, 0x01
ror ebx, 33
cfmerge

; Shift size - Bit Set
mov rbx, 0x1
ror ebx, 32
cfmerge

; Shift size - Bit unset
mov rbx, 0x02
ror ebx, 32
cfmerge

; 64-bit
; Shift 1 past size - Bit Set
mov rbx, 0x02
ror rbx, 65
cfmerge

; Shift 1 past size - Bit unset
mov rbx, 0x8000000000000000
ror rbx, 65
cfmerge

; Shift size - Bit Set
mov rbx, 0x1
ror rbx, 64
cfmerge

; Shift size - Bit unset
mov rbx, 0x02
ror rbx, 64
cfmerge

hlt
