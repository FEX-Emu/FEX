%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x41",
    "RBX": "0xC000000000000000",
    "RCX": "0xFFFF"
  }
}
%endif

; Test reading an empty-tagged x87 register (Stack Fault / Underflow).
; Per Intel SDM Vol 1 Section 8.1.3 & 8.5.1:
; Reading an empty register sets:
; - IE (bit 0 of FSW) = 1 (Invalid Operation)
; - SF (bit 6 of FSW) = 1 (Stack Fault)
; - Masked response substitutes the real QNaN Indefinite (0xFFFFC000000000000000).

fninit

; 1. Push a value to ST(0) then free it so its tag is marked empty
fld1
ffree st0

; 2. Read empty ST(0) into ST(0) via fld st0
; This is a stack underflow: tag is empty.
fld st0

; 3. Check FSW flags: IE (bit 0) and SF (bit 6) must both be set.
; Bit 0 = 0x01, Bit 6 = 0x40 -> 0x41.
fnstsw ax
and rax, 0x41

; 4. Check the loaded value in ST(0) - must be QNaN Indefinite
fstp tword [rel .res_val]

mov rbx, [rel .res_val]        ; 0xC000000000000000
movzx rcx, word [rel .res_val + 8] ; 0xFFFF

hlt

align 4096
.res_val:
dq 0
dw 0
