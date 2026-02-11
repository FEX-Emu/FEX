%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x1"
  }
}
%endif

; Tests undocumented fcom implementation at 0xdc, 0xd0+i
finit
fld1
fldz
; fcom st1
db 0xdc, 0xd1

fnstsw ax
cmp ah, 0x31
je good
mov rax, 0
hlt
good:
mov rax, 1
hlt
