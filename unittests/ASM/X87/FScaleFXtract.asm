%ifdef CONFIG
{
  "RegData": {
    "R8": "1"
  }
}
%endif
; ,
;     "R9": "1",
;     "R10": "1",
;     "R11": "1",
;     "R12": "1"
section .data
    num0: dq 0.0
    num1: dq 125.78
    num2: dq 1023.12
    num3: dq -23487.152
    num4: dq -1230192.123

;; Tests the FScale / FExtract inverse behaviour
section .text
    global _start
_start:
    
; num0 == 0.0
finit
fld qword [rel num0]
fld st0
fxtract
fscale
fstp st1  ; at this point st0 and st1 should be the same
fcom
fnstsw ax
and ax, 0x4500
cmp ax, 0x4000
setz r8b

; ; num1 == 125.78
; finit
; fld qword [rel num1]
; fld st0
; fxtract
; fscale
; fstp st1  ; at this point st0 and st1 should be the same
; fcom
; fnstsw ax
; and ax, 0x4500
; cmp ax, 0x4000
; setz r9b

; ; num2 == 1023.12
; finit
; fld qword [rel num2]
; fld st0
; fxtract
; fscale
; fstp st1  ; at this point st0 and st1 should be the same
; fcom
; fnstsw ax
; and ax, 0x4500
; cmp ax, 0x4000
; setz r10b

; ; num3 == -23487.152
; finit
; fld qword [rel num3]
; fld st0
; fxtract
; fscale
; fstp st1  ; at this point st0 and st1 should be the same
; fcom
; fnstsw ax
; and ax, 0x4500
; cmp ax, 0x4000
; setz r11b

; ; num4 == -1230192.123
; finit
; fld qword [rel num4]
; fld st0
; fxtract
; fscale
; fstp st1  ; at this point st0 and st1 should be the same
; fcom
; fnstsw ax
; and ax, 0x4500
; cmp ax, 0x4000
; setz r12b

hlt
