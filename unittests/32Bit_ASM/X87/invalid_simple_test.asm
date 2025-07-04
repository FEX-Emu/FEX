%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test with a simple 0/0 that we know works
fldz
fldz
fdiv

fstsw ax
and eax, 1

hlt
