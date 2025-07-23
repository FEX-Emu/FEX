%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test 0.0 / 0.0 = Invalid Operation (should set bit 0 of status word)
fldz
fldz
fdiv

fstsw ax
and rax, 1

hlt
