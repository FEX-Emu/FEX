%ifdef CONFIG
{
  "RegData": {
    "RAX":  ["0x0"]
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

fld1
fldz
fcomp
fnstsw ax
test ah, 041h; I *think* I've got the right flags there...
jp good
mov rax, 0
hlt
good:
mov rax, 1
hlt
