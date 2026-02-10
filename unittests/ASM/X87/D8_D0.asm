%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x1"
  }
}
%endif

finit
fld1
fldz
fcom st1

fnstsw ax
cmp ah, 0x31
je good
mov rax, 0
hlt
good:
mov rax, 1
hlt
