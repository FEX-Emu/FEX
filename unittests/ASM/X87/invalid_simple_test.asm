%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test with a simple 0/0 that we know works
fldz
fldz
fdiv

fstsw ax
and rax, 1

hlt
