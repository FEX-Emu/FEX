%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test 0.0 / 0.0 = Invalid Operation (should set bit 0 of status word)
fldz
fldz
fdiv

fstsw ax
and eax, 1

hlt
