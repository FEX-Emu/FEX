%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test FPREM with simple operands first
finit

; Load simple operands: fprem(0, 1) should be valid and return 0
fldz
fld1

; Do FPREM: ST(0) = fprem(ST(0), ST(1)) = fprem(1.0, 0.0)
; fprem(1.0, 0.0) should set Invalid Operation because divisor is zero
fprem

fstsw ax
and rax, 1

hlt
