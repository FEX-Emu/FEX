%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "1"
  },
  "Mode": "32BIT"
}
%endif

finit ; IOC is 0
fldz
fldz
fdiv st0, st1 ; IOC is 1

fnstsw ax
and eax, 1
mov ebx, eax ; save IOC to RBX

; Clear
fnclex

fnstsw ax
and eax, 1

hlt
