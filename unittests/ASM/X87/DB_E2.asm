%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "1"
  }
}
%endif

finit ; IOC is 0
fldz
fldz
fdiv st0, st1 ; IOC is 1

fnstsw ax
and rax, 1
mov rbx, rax ; save IOC to RBX

; Clear
fnclex

fnstsw ax
and rax, 1

hlt
