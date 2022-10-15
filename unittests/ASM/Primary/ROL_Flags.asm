%ifdef CONFIG
{
  "RegData": {
    "R15": "0x0000000001060004"
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
rol bl, 9
cfmerge

; Shift 1 past size - Bit unset
mov rbx, 0x000
rol bl, 9
cfmerge

; Shift size - Bit Set
mov rbx, 0x80
rol bl, 8
cfmerge

; Shift size - Bit unset
mov rbx, 0x8000
rol bl, 8
cfmerge

; 8-bit - wrapped
; Shift 1 past size - Bit Set
mov rbx, 0x01
rol bl, 9
cfmerge

; Shift 1 past size - Bit unset
mov rbx, 0xFFF2
rol bl, 9
cfmerge

; Shift size - Bit Set
mov rbx, 0xFF
rol bl, 8
cfmerge

; Shift size - Bit unset
mov rbx, 0xFF00
rol bl, 8
cfmerge


; 16-bit
; Shift 1 past size - Bit Set
mov rbx, 0x80000
rol bx, 17
cfmerge

; Shift 1 past size - Bit unset
mov rbx, 0x00000
rol bx, 17
cfmerge

; Shift size - Bit Set
mov rbx, 0x8000
rol bx, 16
cfmerge

; Shift size - Bit unset
mov rbx, 0x80000
rol bx, 16
cfmerge

; 32-bit
; Shift 1 past size - Bit Set
mov rbx, 0x800000000
rol ebx, 33
cfmerge

; Shift 1 past size - Bit unset
mov rbx, 0x000000000
rol ebx, 33
cfmerge

; Shift size - Bit Set
mov rbx, 0x80000000
rol ebx, 32
cfmerge

; Shift size - Bit unset
mov rbx, 0x800000000
rol ebx, 32
cfmerge

; 32-bit - Wrapping
; Shift 1 past size - Bit Set
mov rbx, 0x02
rol ebx, 33
cfmerge

; Shift 1 past size - Bit unset
mov rbx, 0x01
rol ebx, 33
cfmerge

; Shift size - Bit Set
mov rbx, 0x1
rol ebx, 32
cfmerge

; Shift size - Bit unset
mov rbx, 0x02
rol ebx, 32
cfmerge

; 64-bit
; Shift 1 past size - Bit Set
mov rbx, 0x02
rol rbx, 65
cfmerge

; Shift 1 past size - Bit unset
mov rbx, 0x8000000000000000
rol rbx, 65
cfmerge

; Shift size - Bit Set
mov rbx, 0x1
rol rbx, 64
cfmerge

; Shift size - Bit unset
mov rbx, 0x02
rol rbx, 64
cfmerge

hlt
