%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "RBX": "1",
    "RCX": "1"
  }
}
%endif

; FSW.IE is sticky
; fcomi, ftst and fist (and fistp, fisttp) should not overwrite IE to 0, when IE is set to 1 already

%macro set_ie 0
  fldz
  fldz
  fdiv st0, st1 ; set IE
%endmacro

finit

set_ie
fldz
fld1
fcomi st1 ; 1.0 vs 0.0
fnstsw ax
and rax, 1 ; extract IE
mov rbx, rax

set_ie
fld1
ftst; 1.0 vs 0.0
fnstsw ax
and rax, 1 ; extract IE
mov rcx, rax

finit
set_ie
fld1
fist word [rel .result]
fnstsw ax
and rax, 1 ; extract IE

hlt

align 4096
.result: dw 0
